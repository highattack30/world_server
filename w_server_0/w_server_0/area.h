#ifndef AREA_H
#define AREA_H

#include "win32.h"
#include "typeDefs.h"
#include "Stream.h"
#include "server_enums.h"

#include <concurrent_queue.h>

#define CELL_ADD_REMOVE_UNLOCK_INSTR_COUNT 20 //TODO

bool WINAPI rectangle_vs_point(float px, float py, float *rectangle);
bool WINAPI rectangle_vs_point(float *p, float *rectangle);
bool WINAPI rectangle_vs_rectangle(float *rectangle1, float *rectangle2);
bool WINAPI rectangle_vs_point(float *p, const int32 *rectangle);
bool WINAPI rectangle_inflates_rectangle(float *rectangle1, float *rectangle2);
bool WINAPI rectangle_inflates_rectangle(float *rectangle1, const int32 * rectangle2);
float WINAPI distance_point_to_point(float p1x, float p1y, float p2x, float p2y);


bool WINAPI square_vs_point(float* square, float* point);
bool WINAPI square_vs_square(float* square, const int32* sq);
bool WINAPI square_vs_rectangle(float*  square, float* rectangle);
bool WINAPI square_vs_rectangle(float*square, const int32* rectangle);


struct player_list {
	struct node { uint32 id; std::shared_ptr<node> next; };
	uint32 count;
	void push_front(uint32);

	void write_to_stream(Stream&);

	std::shared_ptr<node> head;

	player_list();
};

struct bounds {
	bool intersects_bounds(float*, bool point = true) const;
	bool inflates_bounds(float*, bool point = true)const;


	virtual bool load_from_stream(Stream&);

	int32	xywh[4]; //x,y,w,h
};

struct cell : public bounds {
	cell();
	cell(const cell&);
	~cell();

	
	void push_or_remove(std::shared_ptr<player>, bool remove = false);
	uint64 get_count() const;

	void collect_visible(std::shared_ptr<player>, float * p, player_list &p_s, player_list &p_d, player_list &p_m)const;
	void init_player(std::shared_ptr<player>, float*) const;

	void clear(bool server_shutdown = false);
private:
	struct node { std::shared_ptr<player> p_; std::shared_ptr<node> next; };
	struct op_node { std::shared_ptr<node> n_; bool remove; };


	void push_front(std::shared_ptr<node>);
	void remove(entityId);

	CRITICAL_SECTION update_lock;
	concurrency::concurrent_queue<std::shared_ptr<op_node>> to_op;
	std::atomic_int64_t count;
	std::shared_ptr<node> head;
};

struct section : public bounds {
	std::vector<cell> cells;

	void collect_visible(std::shared_ptr<player>, float*, player_list&, player_list&, player_list&) const;
	void init_player(std::shared_ptr<player>, float*) const;

	void clear();
	bool put_player(std::shared_ptr<player>, float*);
	bool remove_player(std::shared_ptr<player>);
	section();
};

struct area : public bounds {
	std::vector<section> sections;

	void collect_visible(std::shared_ptr<player>, float*, player_list&, player_list&, player_list&) const;
	void init_player(std::shared_ptr<player>,  float*) const;

	void clear();
	bool put_player(std::shared_ptr<player>, float*);
	bool remove_player(std::shared_ptr<player>);
	area();
};

bool load_area_from_file(STRING file, area& out_area);
#endif