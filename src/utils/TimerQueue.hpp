#pragma once

#include <queue>
#include <vector>
#include "Time.hpp"

enum TimerKind
{
	T_HEADER,
	T_BODY,
	T_SEND,
	T_KEEPALIVE
};

struct Timer
{
	u64			when_ms;
	int			fd;
	TimerKind	kind;
	u64			gen;
};

// Defining greater operator to change priority_queue to min heap (smallest element at the top)
struct TimerGreater
{
	bool operator()(const Timer& a, const Timer& b) const
	{
		return (a.when_ms > b.when_ms);
	}
};

class TimerQueue
{
public:
	void	add(int fd, TimerKind k, u64 when_ms, u64 gen);
	int		time_to_next(u64 now) const;
	bool	next_due(u64 now, Timer& out);

private:
	std::priority_queue<Timer, std::vector<Timer>, TimerGreater> heap_;
};
