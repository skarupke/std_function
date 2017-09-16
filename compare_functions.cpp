#include <iostream>
#include <random>
#include <string>
#include <chrono>
#include <memory>
#include "function.h"

struct Updateable
{
	virtual ~Updateable() {}
	virtual void update(float dt) = 0;
};

struct UpdateableA : Updateable
{
	UpdateableA()
		: calls(0)
	{
	}
	virtual void update(float) override
	{
		++calls;
	}

	size_t calls;
};
struct UpdateableB : Updateable
{
	virtual void update(float) override
	{
		++calls;
	}
	static size_t calls;
};
size_t UpdateableB::calls = 0;


struct LambdaA
{
	template<typename F>
	explicit LambdaA(std::vector<F> & update_loop)
		: calls(0)
	{
		update_loop.emplace_back([this](float dt) { update(dt); });
	}
	void update(float)
	{
		++calls;
	}
	size_t calls;
};

struct LambdaB
{
	template<typename F>
	explicit LambdaB(std::vector<F> & update_loop)
	{
		update_loop.emplace_back([this](float dt) { update(dt); });
	}
	void update(float)
	{
		++calls;
	}
	static size_t calls;
};
size_t LambdaB::calls = 0;

struct ScopedMeasurer
{
	ScopedMeasurer(std::string name)
		: name(std::move(name))
		, clock()
		, before(clock.now())
	{
	}
	~ScopedMeasurer()
	{
		auto time_spent = std::chrono::duration_cast<std::chrono::milliseconds>(clock.now() - before);
		std::cout << name << ": " << time_spent.count() << " ms" << std::endl;
	}
	std::string name;
	std::chrono::high_resolution_clock clock;
	std::chrono::time_point<std::chrono::high_resolution_clock> before;
};

static const size_t num_allocations = 1000;
#ifdef _DEBUG
static const size_t num_calls = 10000;
#else
static const size_t num_calls = 100000;
#endif

void measure_only_call(const std::vector<Updateable *> & container)
{
	ScopedMeasurer measure("virtual function");
	for (int i = 0; i < num_calls; ++i)
	{
		for (auto & updateable : container)
		{
			updateable->update(0.016f);
		}
	}
}

void time_virtual(int seed)
{
	std::default_random_engine generator(seed);
	std::uniform_int_distribution<int> random_int(0, 1);
	std::vector<std::unique_ptr<Updateable>> updateables;
	std::vector<Updateable *> update_loop;
	for (size_t i = 0; i < num_allocations; ++i)
	{
		if (random_int(generator))
		{
			updateables.emplace_back(new UpdateableA);
		}
		else
		{
			updateables.emplace_back(new UpdateableB);
		}
		update_loop.push_back(updateables.back().get());
	}
	measure_only_call(update_loop);
}

template<typename Container>
void measure_only_call(const Container & container, const std::string & name)
{
	ScopedMeasurer measure(name);
	for (int i = 0; i < num_calls; ++i)
	{
		for (auto & updateable : container)
		{
			updateable(0.016f);
		}
	}
}

template<typename std_function>
void time_std_function(int seed, const std::string & name)
{
	std::default_random_engine generator(seed);
	std::uniform_int_distribution<int> random_int(0, 1);
	std::vector<std_function> update_loop;
	std::vector<std::unique_ptr<LambdaA>> slotsA;
	std::vector<std::unique_ptr<LambdaB>> slotsB;
	for (size_t i = 0; i < num_allocations; ++i)
	{
		if (random_int(generator))
		{
			slotsA.emplace_back(new LambdaA(update_loop));
		}
		else
		{
			slotsB.emplace_back(new LambdaB(update_loop));
		}
	}
	measure_only_call(update_loop, name);
}

int main(int argc, char * argv[])
{
	time_std_function<std::function<void (float)>>(argc, "std::function");
	time_std_function<func::function<void (float)>>(argc, "func::function");
	time_virtual(argc);
}
