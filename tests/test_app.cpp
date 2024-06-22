#include <gmock/gmock.h>

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

int main(int argc, char** argv)
{
    auto& listeners = testing::UnitTest::GetInstance()->listeners();
    delete listeners.Release(listeners.default_result_printer());

    class DoctestListener : public testing::EmptyTestEventListener
    {
    public:
        virtual void OnTestPartResult(const testing::TestPartResult& result) override
        {
            if (!result.failed())
            {
                return;
            }
            const char* file = result.file_name() ? result.file_name() : "unknown";
            const int line = result.line_number() != -1 ? result.line_number() : 0;
            const char* message = result.message() ? result.message() : "no message";
            if (result.nonfatally_failed())
            {
                ADD_FAIL_CHECK_AT(file, line, message);
            }
            else
            {
                ADD_FAIL_AT(file, line, message);
            }
        }
    };
    listeners.Append(new DoctestListener);

    return doctest::Context(argc, argv).run();
}