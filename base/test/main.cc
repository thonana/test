#include <gtest/gtest.h>

#if defined(_MSC_VER)
#include <windows.h>
#endif

int
main(int argc, char** argv)
{
#if defined(_MSC_VER)
    SetConsoleOutputCP(CP_UTF8);
#endif

    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}