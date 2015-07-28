#pragma once

#include <memory>
#include <string>

namespace lct {

class GdbConnectionState;

class GdbConnection {
public:
	GdbConnection();
	virtual ~GdbConnection();

	void Connect(std::string gdb, std::string exec);
	void SetExecutable(std::string elf);
	void DisableTpiu();
	void EnableTpiu(std::string logfile);
	void Run();
	void Stop();

	uint32_t ReadWord(std::string expression, uint32_t* outAddress = NULL);
	uint32_t ReadWord(uint32_t address);
	uint32_t ResolveAddress(std::string expression);
	bool WriteWord(uint32_t address, uint32_t value);
	std::string Evaluate(std::string expression);

protected:
	std::unique_ptr<GdbConnectionState> State;

private:
	GdbConnection(const GdbConnection&);
	GdbConnection& operator=(const GdbConnection&);
};

} // namespace
