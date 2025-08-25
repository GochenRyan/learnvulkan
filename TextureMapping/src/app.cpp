#include <TextureMapping/VulkanApp.h>
#include <iostream>

int main()
{
#ifdef NDEBUG
    std::cout << "Release mode" << std::endl;
#else
    std::cout << "Debug mode" << std::endl;
#endif

    VulkanApp app;

    try
    {
        app.Run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}