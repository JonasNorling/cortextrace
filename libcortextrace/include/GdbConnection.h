#pragma once

#include <memory>

namespace lct {

class GdbConnectionState;

class GdbConnection {
public:
	GdbConnection();
	virtual ~GdbConnection();

	void Connect(std::string gdb, std::string exec);
	void SetExecutable(std::string elf);
	void EnableTpiu();
	void Run();

	uint32_t ReadWord(uint32_t address);

protected:
	std::unique_ptr<GdbConnectionState> State;

private:
	GdbConnection(const GdbConnection&);
	GdbConnection& operator=(const GdbConnection&);
};

} // namespace
