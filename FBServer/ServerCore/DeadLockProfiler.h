#pragma once
#include <stack>
#include <map>
#include <vector>

class DeadLockProfiler
{
public:
	void PushLock(const char* name);
	void PopLock(const char* name);
	void CheckCycle();

private:
	void Dfs(int32 index);

private:
	unordered_map<const char*, int32>	_nameToId;
	unordered_map<int32, const char*>	_idToName;
	map<int32, set<int32>>				_lockHistory;

	// 각 스레드가 독립적인 락 획득 순서를 추적
	static thread_local stack<int32>	_lockStack;

	Mutex _lock;

private:
	vector<int32>		_discoveredOrder;
	int32				_discoveredCount = 0;
	vector<bool>		_finished;
	vector<int32>		_parent;
};
