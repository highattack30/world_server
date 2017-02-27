#include "player.h"
#include "server_enums.h"
#include "world_server.h"

player::player(uint32 id) :eid(id), count(0), head(nullptr) { InitializeCriticalSection(&update_lock); }
player::~player() { DeleteCriticalSection(&update_lock); }

void player::get_position(float *p) const {
	p[0] = a_load(x);
	p[1] = a_load(y);
	p[2] = a_load(z);
}

void player::get_position(float * p, int16 &w_) const {
	p[0] = a_load(x);
	p[1] = a_load(y);
	p[2] = a_load(z);

	w_ = a_load(w);
}

int16 player::get_orientation() const {
	return a_load(w);
}

void player::fill_rectangle(float * p, bool spawn) const {
	float v_range = spawn ? SERVER_VISIBLE_SPAWN_RANGE : SERVER_VISIBLE_DESPAWN_RANGE;
	p[0] = a_load(x) - v_range;
	p[1] = a_load(y) - v_range;
	p[2] = v_range;
}

void player::push_or_remove(uint32 eid, bool remove_) {
	std::shared_ptr<op_node> n_ = std::make_shared<op_node>();
	n_->n_ = std::make_shared<node>();
	n_->n_->next = nullptr;
	n_->remove = remove_;
	n_->n_->p_ = eid;
	to_op.push(n_);
	uint8 t = 0;
	while (1) //small spin lock
	{
		if (t > CELL_ADD_REMOVE_UNLOCK_INSTR_COUNT) break;
		if (TryEnterCriticalSection(&update_lock)) break;
		t++;
	}
	if (t > CELL_ADD_REMOVE_UNLOCK_INSTR_COUNT) return;

	while (1)
	{
		if (!to_op.try_pop(n_)) break;

		if (n_->remove)
			remove(n_->n_->p_);
		else
			push_front(n_->n_);
	}
	LeaveCriticalSection(&update_lock);
}

uint64 player::get_count() const {
	return a_load(count);
}

void player::collect(Stream &data) const {
	data.WriteUInt32(a_load(count));
	std::shared_ptr<node> temp = std::atomic_load(&head);
	while (1) {
		if (!temp) break;
		data.WriteUInt32(temp->p_);
		temp = std::atomic_load(&temp->next);
	}
}

bool player::visible(const uint32 eid) const {
	std::shared_ptr<node> temp = std::atomic_load(&head);
	while (1) {
		if (!temp) break;
		if (temp->p_ == eid) return true;
		temp = std::atomic_load(&temp->next);
	}
	return false;
}

void player::write_to_stream(Stream & data) const{
	std::shared_ptr<node> temp = std::atomic_load(&head);
	while (1) {
		if (!temp) break;
		data.WriteUInt32(temp->p_);
		temp = std::atomic_load(&temp->next);
	}
}

void player::clear() {
	std::shared_ptr<node> temp = std::atomic_load(&head);
	std::shared_ptr<player> t_p;
	while (1) {
		if (!temp) break;
		t_p = w_server_get_player(temp->p_);
		if (t_p)
			t_p->push_or_remove(eid, true);

		temp = std::atomic_load(&temp->next);
	}
}

void player::push_front(std::shared_ptr<node> n) {
	n->next = head;
	while (1) { if (std::atomic_compare_exchange_weak(&head, &head, n)) break; }
	count++;
}

void player::remove(uint32 eid) {
	std::shared_ptr<node> p_0 = nullptr;
	std::shared_ptr<node> p_1 = head;
	while (1)
	{
		if (!p_1) break;

		if (p_1->p_ == eid) {

			if (!p_0) while (1) { if (std::atomic_compare_exchange_weak(&head, &head, head->next))break; }
			else while (1) { if (std::atomic_compare_exchange_weak(&p_0->next, &p_0->next, p_1->next))break; }

			count--;
			break;
		}

		p_0 = p_1;
		p_1 = p_1->next;
	}
}
