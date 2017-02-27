#include "jobs.h"

job_todo::job_todo() { ZeroMemory(this, sizeof(job_todo)); }

job_todo::~job_todo() {
	if (job) delete job;
}

job::job(void * j, e_job_type t) : j_(j), type(t) { ZeroMemory(&ov, sizeof ov); }

job::job() : j_(0), type(J_MAX) { ZeroMemory(&ov, sizeof ov); }

job::~job() {
	if (j_) delete j_;
}

