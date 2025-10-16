#include "TimerQueue.hpp"

void	TimerQueue::add(int fd, TimerKind k, u64 when_ms, u64 gen)
{
	Timer	t;
	t.when_ms = when_ms;
	t.fd = fd;
	t.kind = k;
	t.gen = gen;
	heap_.push(t);
}

/**
 * @return Milliseconds until next timer, -1 if none, 0 if one is now
 */
int	TimerQueue::time_to_next(u64 now) const
{
	if (heap_.empty())
		return (-1);
	const Timer& t = heap_.top();
	if (t.when_ms <= now)
		return (0);
	u64 diff = t.when_ms - now;
	return ((diff > (u64)0x7fffffff) ? 0x7fffffff : (int)diff);
}

/**
 * @brief Pop next due timer
 * @return true if one is popped
 */
bool	TimerQueue::next_due(u64 now, Timer& out)
{
	if (heap_.empty())
		return (false);
	const Timer& t = heap_.top();
	if (t.when_ms > now)
		return (false);
	out = t;
	heap_.pop();
	return (true);
}
