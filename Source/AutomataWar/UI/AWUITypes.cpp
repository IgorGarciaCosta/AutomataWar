#include "AWUITypes.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

DEFINE_LOG_CATEGORY(LogAutomataUI);

namespace
{
    FSlateFontInfo FontFromContent(const TCHAR *RelativePath, int32 Size, const FName &FallbackStyle)
    {
        const FString FontPath = FPaths::ProjectContentDir() / RelativePath;
        return IFileManager::Get().FileExists(*FontPath)
                   ? FSlateFontInfo(FontPath, Size)
                   : FCoreStyle::GetDefaultFontStyle(FallbackStyle, Size);
    }
}

FSlateFontInfo AWUIFonts::Mono(int32 Size)
{
    return FontFromContent(AWUIAssets::MonoFontFile, Size, TEXT("Mono"));
}

FSlateFontInfo AWUIFonts::Display(int32 Size)
{
    return FontFromContent(AWUIAssets::DisplayFontFile, Size, TEXT("Bold"));
}
