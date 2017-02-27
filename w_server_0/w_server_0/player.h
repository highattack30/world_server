#ifndef PLAYER_H
#define PLAYER_H

#include "typeDefs.h"
#include "win32.h"
#include "Stream.h"

#include <concurrent_queue.h>

#ifndef CELL_ADD_REMOVE_UNLOCK_INSTR_COUNT
#define CELL_ADD_REMOVE_UNLOCK_INSTR_COUNT 20
#endif

struct player {
	player(uint32);
	~player();
	const uint32  eid;

	std::atomic<uint32> section_i;
	std::atomic<uint32> cell_i;

	std::atomic<float> x;
	std::atomic<float> y;
	std::atomic<float> z;
	std::atomic<int16> w;

	void get_position(float*) const;
	void get_position(float*, int16&) const;
	int16 get_orientation() const;
	void fill_rectangle(float*, bool spawn) const;

	void push_or_remove(uint32, bool remove = false);
	uint64 get_count() const;
	void collect(Stream&) const;
	bool visible(const uint32)const;
	void write_to_stream(Stream&) const;

	void clear();
private:
	struct node { uint32 p_; std::shared_ptr<node> next; };
	struct op_node { std::shared_ptr<node> n_; bool remove; };


	void push_front(std::shared_ptr<node>);
	void remove(uint32);

	CRITICAL_SECTION update_lock;
	concurrency::concurrent_queue<std::shared_ptr<op_node>> to_op;
	std::atomic_int64_t count;
	std::shared_ptr<node> head;
};
#endif
