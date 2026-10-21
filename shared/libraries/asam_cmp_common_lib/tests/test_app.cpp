#include <opendaq/module_manager_factory.h>

#include <testutils/testutils.h>
#include <testutils/daq_memcheck_listener.h>

#ifdef _WIN32
#include <crtdbg.h>
#include <stdlib.h>
#endif

int main(int argc, char** args)
{
    using namespace daq;

#ifdef _WIN32
    // report CRT asserts and abort() to stderr instead of a dialog that nobody closes on CI
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif

    {
        daq::ModuleManager(".");
    }

    testing::InitGoogleTest(&argc, args);

    testing::TestEventListeners& listeners = testing::UnitTest::GetInstance()->listeners();
    listeners.Append(new DaqMemCheckListener());

    return RUN_ALL_TESTS();
}
