/***************************************************************************
 *   Copyright (C) 2009 by Andrey Afletdinov <fheroes2@gmail.com>          *
 *                                                                         *
 *   Part of the Free Heroes2 Engine:                                      *
 *   http://sourceforge.net/projects/fheroes2                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <iostream>

#include "engine.h"
#include "gamedefs.h"
#include "settings.h"
#include "agg.h"
#include "cursor.h"
#include "game.h"
#include "display.h"
#include "system.h"
#include "tools.h"
#include "rand.h"
#include "audio_mixer.h"
#include "audio_music.h"
#include "icn.h"
#include "text.h"

void LoadZLogo();

void SetVideoDriver(const string&);

void SetTimidityEnvPath(const Settings&);

void SetLangEnvPath(const Settings&);

void InitHomeDir();

void ReadConfigs();

int TestBlitSpeed();

int PrintHelp(const char* basename)
{
    COUT("Usage: " << basename << " [OPTIONS]");
#ifndef BUILD_RELEASE
    COUT("  -d\tdebug mode");
#endif
    COUT("  -h\tprint this help and exit");

    return EXIT_SUCCESS;
}

void extractFrames()
{
    return;
    static bool isExtracted = false;
    if (isExtracted)
        return;
    for (int icnId = 3; icnId < ICN::LASTICN; icnId++)
    {
        bool canContinue = false;
        int frameId = 0;
        do
        {
            AGG::ICNSprite sprite;
            try
            {
                sprite = AGG::RenderICNSprite(icnId, frameId);
            }
            catch (...)
            {
                break;
            }
            frameId++;
            canContinue = sprite.isValid();
        }
        while (canContinue);
    }
    isExtracted = true;
}

string GetCaption()
{
    return string("Free Heroes II, version: " + Settings::GetVersion());
}

std::vector<std::string> extractArgsVector(int argc, char** argv);

#ifdef __APPLE__
int SDL_main(int argc, char **argv)
{
#elif WIN32

struct HINSTANCE__
{
    int unused;
};

int __stdcall wWinMain(HINSTANCE__* hInstance, HINSTANCE__* hPrevInstance, wchar_t* pCmdLine, int nCmdShow)
{
    int argc = 0;
    char** argv = nullptr;


#else

int main(int argc, char **argv)
{
#endif

    vector<string> vArgv = extractArgsVector(argc, argv);

    Settings& conf = Settings::Get();

    conf.SetProgramPath(vArgv[0]);

    InitHomeDir();
    ReadConfigs();

    // getopt
    {
        int opt;
        while ((opt = System::GetCommandOptions(vArgv.size(), vArgv, "ht:d:")) != -1)
            switch (opt)
            {
#ifndef BUILD_RELEASE
                case 't':
        test = GetInt(System::GetOptionsArgument());
        break;

                case 'd':
                conf.SetDebug(System::GetOptionsArgument() ? GetInt(System::GetOptionsArgument()) : 0);
                break;
#endif
            case '?':
            case 'h':
                return PrintHelp(vArgv[0].c_str());

            default:
                break;
            }
    }

    if (!conf.SelectVideoDriver().empty()) SetVideoDriver(conf.SelectVideoDriver());

    // random init
    Rand::Init();
    if (conf.Music()) SetTimidityEnvPath(conf);

    uint32_t subsystem = INIT_VIDEO | INIT_TIMER;

    if (conf.Sound() || conf.Music())
        subsystem |= INIT_AUDIO;

    if (SDL::Init(subsystem))
    {
        atexit(SDL::Quit);

        SetLangEnvPath(conf);

        if (Mixer::isValid())
        {
            Mixer::SetChannels(8);
            Mixer::Volume(-1, Mixer::MaxVolume() * conf.SoundVolume() / 10);
            Music::Volume(Mixer::MaxVolume() * conf.MusicVolume() / 10);
            if (conf.Music())
            {
                Music::SetFadeIn(3000);
            }
        }
        else if (conf.Sound() || conf.Music())
        {
            conf.ResetSound();
            conf.ResetMusic();
        }

        if (0 == conf.VideoMode().w || 0 == conf.VideoMode().h)
            conf.SetAutoVideoMode();

        Display& display = Display::Get();
        display.SetVideoMode(conf.VideoMode().w, conf.VideoMode().h, conf.FullScreen());
        Display::HideCursor();
        Display::SetCaption(GetCaption().c_str());

        //Ensure the mouse position is updated to prevent bad initial values.
        LocalEvent::Get().GetMouseCursor();


        // read data dir
        if (!AGG::Init())
            return EXIT_FAILURE;

        atexit(&AGG::Quit);

        try
        {
            AGG::GetICN(ICN::DRAGRED, 0);
        }
        catch (...)
        {
            std::cout << "Cannot load picture. Make sure you have Heroes 2 assets extracted here" << "\n";

            std::cout << "If no Heroes 2 is accessible, you can use Heroes 2 Demo files\n"
                << "at address: "
                << "https://github.com/ciplogic/fheroes2enh/releases/download/0.9.1/h2demo.zip" << "\n"
                << " Quitting" << "\n";

            return -1;
        }

        Text loadingTextMessage = { "Building maps file list..." };
        loadingTextMessage.Blit((display.w()-loadingTextMessage.w())/2, display.h() / 2, Display::Get());
        display.Flip();

        Maps::PrepareFilesCache();
        //extractFrames();
        
        conf.SetBlitSpeed(TestBlitSpeed());

        // init cursor
        Cursor::Get().SetThemes(Cursor::POINTER);

        // init game data
        Game::Init();

        int test = 0;

        // goto main menu
        int rs = test ? Game::TESTING : Game::MAINMENU;

        while (rs != Game::QUITGAME)
        {
            switch (rs)
            {
            case Game::MAINMENU:
                extractFrames();
                rs = Game::MainMenu();
                break;
            case Game::NEWGAME:
                rs = Game::NewGame();
                break;
            case Game::LOADGAME:
                rs = Game::LoadGame();
                break;
            case Game::HIGHSCORES:
                rs = Game::HighScores(true);
                break;
            case Game::CREDITS:
                rs = Game::Credits();
                break;
            case Game::NEWSTANDARD:
                rs = Game::NewStandard();
                break;
            case Game::NEWCAMPAIN:
                rs = Game::NewCampain();
                break;
            case Game::NEWMULTI:
                rs = Game::NewMulti();
                break;
            case Game::NEWHOTSEAT:
                rs = Game::NewHotSeat();
                break;
#ifdef NETWORK_ENABLE
            case Game::NEWNETWORK:
                rs = Game::NewNetwork();
                break;
#endif
            case Game::NEWBATTLEONLY:
                rs = Game::NewBattleOnly();
                break;
            case Game::LOADSTANDARD:
                rs = Game::LoadStandard();
                break;
            case Game::LOADCAMPAIN:
                rs = Game::LoadCampain();
                break;
            case Game::LOADMULTI:
                rs = Game::LoadMulti();
                break;
            case Game::SCENARIOINFO:
                rs = Game::ScenarioInfo();
                break;
            case Game::SELECTSCENARIO:
                rs = Game::SelectScenario();
                break;
            case Game::STARTGAME:
                rs = Game::StartGame();
                break;
            case Game::TESTING:
                rs = Game::Testing(test);
                break;

            default:
                break;
            }
        }
    }
    return EXIT_SUCCESS;
}

int TestBlitSpeed()
{
    Display& display = Display::Get();
    Surface sf(display.GetSize(), true);
    Rect srcrt(0, 0, display.w() / 3, display.h());
    SDL::Time t;

    t.Start();
    sf.Fill(RGBA(0xFF, 0, 0));
    sf.Blit(srcrt, Point(0, 0), display);
    display.Flip();
    sf.Fill(RGBA(0, 0xFF, 0));
    sf.Blit(srcrt, Point(srcrt.w, 0), display);
    display.Flip();
    sf.Fill(RGBA(0, 0, 0xFF));
    sf.Blit(srcrt, Point(display.w() - srcrt.w, 0), display);
    display.Flip();
    sf.Fill(RGBA(0, 0, 0));
    sf.Blit(display);
    display.Flip();
    t.Stop();

    int res = t.Get();
    return res;
}

void LoadZLogo()
{
#ifdef BUILD_RELEASE
    string file = Settings::GetLastFile("image", "sdl_logo.png");
    // SDL logo
    if (Settings::Get().ExtGameShowSDL() && !file.empty())
    {
        Display& display = Display::Get();
        Surface sf;

        if (sf.Load(file))
        {
            Surface black(display.GetSize(), false);
            black.Fill(ColorBlack);

            const Point offset((display.w() - sf.w()) / 2, (display.h() - sf.h()) / 2);

            display.Rise(sf, black, offset, 250, 500);
            display.Fade(sf, black, offset, 10, 500);
        }
    }
#endif
}

void ReadConfigs()
{
    Settings& conf = Settings::Get();
    ListFiles files = Settings::GetListFiles("", "fheroes2.cfg");

    for (auto& file : files)
    {
        if (System::IsFile(file))
        {
            conf.Read(file);
        }
    }
}

void InitHomeDir()
{
    const string home = System::GetHomeDirectory("fheroes2");

    if (!home.empty())
    {
        const string home_maps = System::ConcatePath(home, "maps");
        const string home_files = System::ConcatePath(home, "files");
        const string home_files_save = System::ConcatePath(home_files, "save");

        if (!System::IsDirectory(home))
            System::MakeDirectory(home);

        if (System::IsDirectory(home, true) && !System::IsDirectory(home_maps))
            System::MakeDirectory(home_maps);

        if (System::IsDirectory(home, true) && !System::IsDirectory(home_files))
            System::MakeDirectory(home_files);

        if (System::IsDirectory(home_files, true) && !System::IsDirectory(home_files_save))
            System::MakeDirectory(home_files_save);
    }
}

void SetVideoDriver(const string& driver)
{
    System::SetEnvironment("SDL_VIDEODRIVER", driver.c_str());
}

void SetTimidityEnvPath(const Settings& conf)
{
    const string prefix_timidity = System::ConcatePath("files", "timidity");
    const string result = Settings::GetLastFile(prefix_timidity, "timidity.cfg");

    if (System::IsFile(result))
        System::SetEnvironment("TIMIDITY_PATH", System::GetDirname(result).c_str());
}

void SetLangEnvPath(const Settings& conf)
{
    System::SetLocale(LC_ALL, "");
    System::SetLocale(LC_NUMERIC, "C");

    std::string mofile = conf.ForceLang().empty()
                             ? System::GetMessageLocale(1).append(".po")
                             : std::string(conf.ForceLang()).append(".po");

    ListFiles translations = Settings::GetListFiles(System::ConcatePath("files", "lang"), mofile);

    if (!translations.empty())
    {
        Translation::bindDomain(translations.back().c_str());
    }
    else
    H2ERROR("translation not found: " + mofile);
}
