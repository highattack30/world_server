#include "area.h"
#include "player.h"

#include <fstream>

//rectangle
bool WINAPI rectangle_vs_point(float px, float py, float* rectangle)
{
	return(((rectangle[0] + rectangle[2]) > px) && ((rectangle[1] + rectangle[3]) > py) && (rectangle[0] < px) && (rectangle[1] < py));
}

bool WINAPI rectangle_vs_point(float * p, float *rectangle)
{
	return(((rectangle[0] + rectangle[2]) > p[0]) && ((rectangle[1] + rectangle[3]) > p[1]) && (rectangle[0] < p[0]) && (rectangle[1] < p[1]));
}

bool WINAPI rectangle_vs_point(float * p, const int32 *rectangle)
{
	return(((rectangle[0] + rectangle[2]) > p[0]) && ((rectangle[1] + rectangle[3]) > p[1]) && (rectangle[0] < p[0]) && (rectangle[1] < p[1]));
}

bool WINAPI rectangle_vs_rectangle(float *rectangle1, float *rectangle2)
{
	return ((rectangle1[0] < (rectangle2[0] + rectangle2[2])) && ((rectangle1[0] + rectangle1[2]) > rectangle2[0]) &&
		(rectangle1[1] < (rectangle2[1] + rectangle2[3])) && ((rectangle1[1] + rectangle1[3] > rectangle2[1])));
}

bool WINAPI rectangle_vs_rectangle(float* rectangle1, const int32* rectangle2)
{
	return ((rectangle1[0] < (rectangle2[0] + rectangle2[2])) && ((rectangle1[0] + rectangle1[2]) > rectangle2[0]) &&
		(rectangle1[1] < (rectangle2[1] + rectangle2[3])) && ((rectangle1[1] + rectangle1[2] > rectangle2[1])));
}

bool WINAPI rectangle_inflates_rectangle(float *rectangle1, float *rectangle2)
{
	return
		(
		(((rectangle1[0] + rectangle1[2]) < (rectangle2[0] + rectangle2[2])) && ((rectangle1[0] + rectangle1[2]) > rectangle2[0])) ||
			(((rectangle1[1] + rectangle1[3]) < (rectangle2[1] + rectangle2[3])) && ((rectangle1[1] + rectangle1[3]) > rectangle2[1])) ||
			((rectangle1[0] > rectangle2[0]) && (rectangle1[0] < (rectangle2[0] + rectangle2[2]))) ||
			((rectangle1[1] > rectangle2[1]) && (rectangle1[1] < (rectangle2[1] + rectangle2[3])))
			);
}

bool WINAPI rectangle_inflates_rectangle(float *rectangle1, const int32 *rectangle2)
{
	return
		(
		(((rectangle1[0] + rectangle1[2]) < (rectangle2[0] + rectangle2[2])) && ((rectangle1[0] + rectangle1[2]) > rectangle2[0])) ||
			(((rectangle1[1] + rectangle1[3]) < (rectangle2[1] + rectangle2[3])) && ((rectangle1[1] + rectangle1[2]) > rectangle2[1])) ||
			((rectangle1[0] > rectangle2[0]) && (rectangle1[0] < (rectangle2[0] + rectangle2[2]))) ||
			((rectangle1[1] > rectangle2[1]) && (rectangle1[1] < (rectangle2[1] + rectangle2[3])))
			);
}


//square
bool WINAPI square_vs_point(float * square, float * p) {
	return(((square[0] + square[2]) > p[0]) && ((square[1] + square[2]) > p[1]) && (square[0] < p[0]) && (square[1] < p[1]));
}

bool WINAPI square_vs_square(float * square, const int32 * sq){
	return ((square[0] < (sq[0] + sq[2])) && ((square[0] + square[2]) > sq[0]) &&
		(square[1] < (sq[1] + sq[2])) && ((square[1] + square[2] > sq[1])));
}

bool WINAPI square_vs_rectangle(float *square, float * rectangle) {
	return ((square[0] < (rectangle[0] + rectangle[2])) && ((square[0] + square[2]) > rectangle[0]) &&
		(square[1] < (rectangle[1] + rectangle[3])) && ((square[1] + square[2] > rectangle[1])));
}

bool WINAPI square_vs_rectangle(float * square, const int32 * rectangle) {
	return ((square[0] < (rectangle[0] + rectangle[2])) && ((square[0] + square[2]) > rectangle[0]) &&
		(square[1] < (rectangle[1] + rectangle[3])) && ((square[1] + square[2] > rectangle[1])));
}

float WINAPI distance_point_to_point(float p1x, float p1y, float p2x, float p2y)
{
	float a = p1x - p2x;
	float b = p1y - p2y;
	return sqrt(a * a + b * b);
}



bool bounds::intersects_bounds(float *p, bool point)const {
	if (point)
		return(((xywh[0] + xywh[2]) > p[0]) && ((xywh[1] + xywh[3]) > p[1]) && (xywh[0] < p[0]) && (xywh[1] < p[1]));
	else
		return square_vs_rectangle(p, xywh);
}

bool bounds::inflates_bounds(float *p, bool point)const {
	if (point)
		return !rectangle_vs_point(p, xywh);
	else
		return rectangle_inflates_rectangle(p, xywh);
}

bool bounds::load_from_stream(Stream & data) {
	xywh[0] = data.ReadInt32();
	xywh[1] = data.ReadInt32();
	xywh[2] = data.ReadInt32();
	xywh[3] = data.ReadInt32();
	return true;
}







//AREA----------------------------------------------------------------
area::area() {}
void area::collect_visible(std::shared_ptr<player> p_, float * p, player_list &p_s, player_list &p_d, player_list &p_m) const {
	for (size_t i = 0; i < sections.size(); i++) {
		if (rectangle_vs_rectangle(&p[12], sections[i].xywh))
			sections[i].collect_visible(p_, p, p_s, p_d, p_m);
	}
}

void area::init_player(std::shared_ptr<player> p, float * p_r) const {
	for (size_t i = 0; i < sections.size(); i++) {
		if (sections[i].intersects_bounds(p_r))
			sections[i].init_player(p, p_r);
	}
}

void area::clear() {
	for (size_t i = 0; i < sections.size(); i++)
		sections[i].clear();
}
bool area::put_player(std::shared_ptr<player> p, float * p_r) {
	for (size_t i = 0; i < sections.size(); i++) {
		if (sections[i].intersects_bounds(p_r)) {

			if (p->section_i != i)
				printf("changed section %d %d\n", p->section_i.load(), i);
			p->section_i = i;
			return sections[i].put_player(p, p_r);
		}
	}
	return false;
}
bool area::remove_player(std::shared_ptr<player> p) {
	return sections[p->section_i].remove_player(p);
}
//AREA----------------------------------------------------------------



//SECTION----------------------------------------------------------------
section::section() {}
void section::collect_visible(std::shared_ptr<player> p_, float * p, player_list &p_s, player_list &p_d, player_list &p_m) const {
	for (size_t i = 0; i < cells.size(); i++) {
		if (rectangle_vs_rectangle(&p[12], cells[i].xywh)) {
			cells[i].collect_visible(p_, p, p_s, p_d, p_m);
		}
	}
}
void section::init_player(std::shared_ptr<player> p, float * p_r) const {
	for (size_t i = 0; i < cells.size(); i++) {
		if (rectangle_vs_rectangle(p_r, cells[i].xywh)) {
			cells[i].init_player(p, p_r);
		}
	}
}
void section::clear() {
	for (size_t i = 0; i < cells.size(); i++)
		cells[i].clear(true);
}

bool section::put_player(std::shared_ptr<player> p, float* p_r) {
	for (size_t i = 0; i < cells.size(); i++) {
		if (cells[i].intersects_bounds(p_r)) {
			cells[i].push_or_remove(p);
			p->cell_i = i;
			return true;
		}
	}
	return false;
}
bool section::remove_player(std::shared_ptr<player> p) {
	cells[p->cell_i].push_or_remove(p, true);
	return true;
}
//SECTION----------------------------------------------------------------

//CELL----------------------------------------------------------------

cell::cell() : head(nullptr), count(0) { InitializeCriticalSection(&update_lock); }
cell::cell(const cell & c) : head(nullptr), count(0) { xywh[0] = c.xywh[0]; xywh[1] = c.xywh[1]; xywh[2] = c.xywh[2]; xywh[3] = c.xywh[3]; InitializeCriticalSection(&update_lock); }

cell::~cell() {
	DeleteCriticalSection(&update_lock);

	std::shared_ptr<node> temp = head;
	head = nullptr;
	while (1)
	{
		if (!temp) break;
		temp = temp->next;
	}
}

void cell::push_or_remove(std::shared_ptr<player> p, bool remove_) {
	std::shared_ptr<op_node> n_ = std::make_shared<op_node>();
	n_->n_ = std::make_shared<node>();
	n_->n_->next = nullptr;
	n_->remove = remove_;
	n_->n_->p_ = p;
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
			remove(n_->n_->p_->eid);
		else
			push_front(n_->n_);
	}
	LeaveCriticalSection(&update_lock);
}

uint64 cell::get_count() const {
	return a_load(count);
}
void cell::collect_visible(std::shared_ptr<player> p, float * p_r, player_list &p_s, player_list &p_d, player_list &p_m) const {
	std::shared_ptr<node> temp = std::atomic_load(&head);
	bool see_spawn, see_despawn, visible;
	while (1) {
		if (!temp) break;
		if (temp->p_->eid != p->eid) {
			temp->p_->get_position(&p_r[6]);
			see_spawn = square_vs_point(p_r, &p_r[6]);
			see_despawn = square_vs_point(&p_r[3], &p_r[6]);
			visible = p->visible(temp->p_->eid);

			if (!see_despawn && visible) {
				p_d.push_front(temp->p_->eid);
				p->push_or_remove(temp->p_->eid, true);
				temp->p_->push_or_remove(p->eid, true);
			}
			else if (see_spawn && !visible) {
				p_s.push_front(temp->p_->eid);
				p->push_or_remove(temp->p_->eid);
				temp->p_->push_or_remove(p->eid);
			}
			else if (see_despawn  && visible) {
				p_m.push_front(temp->p_->eid);
			}
		}
		temp = std::atomic_load(&temp->next);
	}
}

void cell::init_player(std::shared_ptr<player> p, float *p_r) const {
	std::shared_ptr<node> temp = std::atomic_load(&head);
	while (1) {
		if (!temp) break;
		if (temp->p_->eid != p->eid) {
			temp->p_->get_position(&p_r[3]);
			if (square_vs_point(p_r, &p_r[3])) {
				temp->p_->push_or_remove(p->eid);
				p->push_or_remove(temp->p_->eid);
			}
		}
		temp = std::atomic_load(&temp->next);
	}
}

void cell::clear(bool server_shutdown)
{
	//todo
	if (server_shutdown) {

	}
	else {

	}
}

void cell::push_front(std::shared_ptr<node> n) {

	n->next = head;
	while (1) { if (std::atomic_compare_exchange_weak(&head, &head, n)) break; }
	count++;
}

void cell::remove(entityId id) {
	std::shared_ptr<node> p_0 = nullptr;
	std::shared_ptr<node> p_1 = head;
	while (1)
	{
		if (!p_1) break;

		if (p_1->p_->eid == id) {

			if (!p_0) while (1) { if (std::atomic_compare_exchange_weak(&head, &head, head->next))break; }
			else while (1) { if (std::atomic_compare_exchange_weak(&p_0->next, &p_0->next, p_1->next))break; }

			count--;
			break;
		}

		p_0 = p_1;
		p_1 = p_1->next;
	}
}

void player_list::push_front(uint32 id) {
	std::shared_ptr<node> n = std::make_shared<node>();
	n->id = id;
	n->next = head;
	head = n;

	count++;
}

void player_list::write_to_stream(Stream & data)
{
	std::shared_ptr<node> temp = head;
	while (1)
	{
		if (!temp) break;
		data.WriteUInt32((uint32)temp->id);
		temp = temp->next;
	}
}

player_list::player_list() :head(nullptr), count(0) {}

//CELL----------------------------------------------------------------


bool load_area_from_file(STRING file_name, area & out_area) {
	std::ifstream file = std::ifstream(file_name, std::ifstream::binary);
	if (!file.is_open() || !file.good()) {
		return false;
	}
	Stream data = Stream();
	file.seekg(0, std::fstream::end);
	data.Resize((uint16)file.tellg());
	file.seekg(0, std::fstream::beg);
	file.read((char*)data._raw, data._size);
	file.close();

	if (data._size < 4) {
		return false;
	}

	uint32 section_count = data.ReadUInt32();
	out_area.load_from_stream(data);
	uint32 cell_count = 0, total_cell_count = 0;
	for (uint32 i = 0; i < section_count; i++) {
		cell_count = data.ReadUInt32();
		total_cell_count += cell_count;
		section s;
		s.load_from_stream(data);

		for (uint32 j = 0; j < cell_count; j++) {
			cell c;
			c.load_from_stream(data);
			s.cells.push_back(c);
		}

		out_area.sections.push_back(s);
	}

	printf("LOADED [%s] SECTION_COUNT[%d]  TOTAL_CELL_COUNT[%d]\n", file_name, (uint32)out_area.sections.size(), total_cell_count);

	return true;
}