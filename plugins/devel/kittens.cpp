#include <dfhack/Core.h>
#include <dfhack/Console.h>
#include <dfhack/Export.h>
#include <dfhack/PluginManager.h>
#include <vector>
#include <string>
#include "dfhack/extra/stopwatch.h"
#include "dfhack/modules/Maps.h"
#include "dfhack/modules/Items.h"
#include <dfhack/modules/Gui.h>

using std::vector;
using std::string;
using namespace DFHack;
//FIXME: possible race conditions with calling kittens from the IO thread and shutdown from Core.
bool shutdown_flag = false;
bool final_flag = true;
bool timering = false;
bool trackmenu_flg = false;
uint32_t last_menu = 0;
uint64_t timeLast = 0;

DFhackCExport command_result kittens (Core * c, vector <string> & parameters);
DFhackCExport command_result ktimer (Core * c, vector <string> & parameters);
DFhackCExport command_result bflags (Core * c, vector <string> & parameters);
DFhackCExport command_result trackmenu (Core * c, vector <string> & parameters);
DFhackCExport command_result mapitems (Core * c, vector <string> & parameters);
DFhackCExport command_result test_creature_offsets (Core * c, vector <string> & parameters);
DFhackCExport command_result creat_job (Core * c, vector <string> & parameters);

DFhackCExport const char * plugin_name ( void )
{
    return "kittens";
}

DFhackCExport command_result plugin_init ( Core * c, std::vector <PluginCommand> &commands)
{
    commands.clear();
    commands.push_back(PluginCommand("nyan","NYAN CAT INVASION!",kittens, true));
    commands.push_back(PluginCommand("ktimer","Measure time between game updates and console lag (toggle).",ktimer));
    commands.push_back(PluginCommand("blockflags","Look up block flags",bflags));
    commands.push_back(PluginCommand("trackmenu","Track menu ID changes (toggle).",trackmenu));
    commands.push_back(PluginCommand("mapitems","Check item ids under cursor against item ids in map block.",mapitems));
    commands.push_back(PluginCommand("test_creature_offsets","Bleh.",test_creature_offsets));
    commands.push_back(PluginCommand("creat_job","Bleh.",creat_job));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( Core * c )
{
    shutdown_flag = true;
    while(!final_flag)
    {
        c->con.msleep(60);
    }
    return CR_OK;
}

DFhackCExport command_result plugin_onupdate ( Core * c )
{
    if(timering == true)
    {
        uint64_t time2 = GetTimeMs64();
        // harmless potential data race here...
        uint64_t delta = time2-timeLast;
        // harmless potential data race here...
        timeLast = time2;
        c->con.print("Time delta = %d ms\n", delta);
    }
    if(trackmenu_flg)
    {
        DFHack::Gui * g =c->getGui();
        if (last_menu != *g->df_menu_state)
        {
            last_menu = *g->df_menu_state;
            c->con.print("Menu: %d\n",last_menu);
        }
    }
    return CR_OK;
}

DFhackCExport command_result mapitems (Core * c, vector <string> & parameters)
{
    c->Suspend();
    vector <t_item *> vec_items;
    Gui * g = c-> getGui();
    Maps* m = c->getMaps();
    Items* it = c->getItems();
    if(!m->Start())
    {
        c->con.printerr("No map to probe\n");
        return CR_FAILURE;
    }
    if(!it->Start() || !it->readItemVector(vec_items))
    {
        c->con.printerr("Failed to get items\n");
        return CR_FAILURE;
    }
    int32_t cx,cy,cz;
    g->getCursorCoords(cx,cy,cz);
    if(cx != -30000)
    {
        df_block * b = m->getBlock(cx/16,cy/16,cz);
        if(b)
        {
            c->con.print("Items in block:\n");
            auto iter_b = b->items.begin();
            while (iter_b != b->items.end())
            {
                c->con.print("%d\n",*iter_b);
                iter_b++;
            }
            c->con.print("Items under cursor:\n");
            auto iter_it = vec_items.begin();
            while (iter_it != vec_items.end())
            {
                t_item * itm = *iter_it;
                if(itm->x == cx && itm->y == cy && itm->z == cz)
                {
                    c->con.print("%d\n",itm->id);
                }
                iter_it ++;
            }
        }
    }
    c->Resume();
    return CR_OK;
}

DFhackCExport command_result trackmenu (Core * c, vector <string> & parameters)
{
    if(trackmenu_flg)
    {
        trackmenu_flg = false;
        return CR_OK;
    }
    else
    {
        DFHack::Gui * g =c->getGui();
        if(g->df_menu_state)
        {
            trackmenu_flg = true;
            last_menu =  *g->df_menu_state;
            c->con.print("Menu: %d\n",last_menu);
            return CR_OK;
        }
        else
        {
            c->con.printerr("Can't read menu state\n");
            return CR_FAILURE;
        }
    }
}

DFhackCExport command_result bflags (Core * c, vector <string> & parameters)
{
    c->Suspend();
    Gui * g = c-> getGui();
    Maps* m = c->getMaps();
    if(!m->Start())
    {
        c->con.printerr("No map to probe\n");
        return CR_FAILURE;
    }
    int32_t cx,cy,cz;
    g->getCursorCoords(cx,cy,cz);
    if(cx == -30000)
    {
        // get map size in blocks
        uint32_t sx,sy,sz;
        m->getSize(sx,sy,sz);
        std::map <uint8_t, df_block *> counts;
        // for each block
        for(size_t x = 0; x < sx; x++)
            for(size_t y = 0; y < sx; y++)
                for(size_t z = 0; z < sx; z++)
                {
                    df_block * b = m->getBlock(x,y,z);
                    if(!b) continue;
                    auto iter = counts.find(b->flags.size);
                    if(iter == counts.end())
                    {
                        counts[b->flags.bits[0]] = b;
                    }
                }
        for(auto iter = counts.begin(); iter != counts.end(); iter++)
        {
            c->con.print("%2x : 0x%x\n",iter->first, iter->second);
        }
    }
    else
    {
        df_block * b = m->getBlock(cx/16,cy/16,cz);
        if(b)
        {
            c->con << "Block flags:" << b->flags << std::endl;
        }
        else
        {
            c->con.printerr("No block here\n");
            return CR_FAILURE;
        }
    }
    c->Resume();
    return CR_OK;
}

DFhackCExport command_result ktimer (Core * c, vector <string> & parameters)
{
    if(timering)
    {
        timering = false;
        return CR_OK;
    }
    uint64_t timestart = GetTimeMs64();
    c->Suspend();
    c->Resume();
    uint64_t timeend = GetTimeMs64();
    c->con.print("Time to suspend = %d ms\n",timeend - timestart);
    // harmless potential data race here...
    timeLast = timeend;
    timering = true;
    return CR_OK;
}

DFhackCExport command_result kittens (Core * c, vector <string> & parameters)
{
    final_flag = false;
    Console & con = c->con;
    // http://evilzone.org/creative-arts/nyan-cat-ascii/
    const char * nyan []=
    {
        "NYAN NYAN NYAN NYAN NYAN NYAN NYAN",
        "+      o     +              o   ",
        "    +             o     +       +",
        "o          +",
        "    o  +           +        +",
        "+        o     o       +        o",
        "-_-_-_-_-_-_-_,------,      o ",
        "_-_-_-_-_-_-_-|   /\\_/\\  ",
        "-_-_-_-_-_-_-~|__( ^ .^)  +     +  ",
        "_-_-_-_-_-_-_-\"\"  \"\"      ",
        "+      o         o   +       o",
        "    +         +",
        "o        o         o      o     +",
        "    o           +",
        "+      +     o        o      +    ",
        "NYAN NYAN NYAN NYAN NYAN NYAN NYAN",
        0
    };
    const char * kittenz1 []=
    {
        "   ____",
        "  (.   \\",
        "    \\  |  ",
        "     \\ |___(\\--/)",
        "   __/    (  . . )",
        "  \"'._.    '-.O.'",
        "       '-.  \\ \"|\\",
        "          '.,,/'.,,mrf",
        0
    };
    con.cursor(false);
    con.clear();
    Console::color_value color = Console::COLOR_BLUE;
    while(1)
    {
        if(shutdown_flag)
        {
            final_flag = true;
            con.reset_color();
            con << std::endl << "NYAN!" << std::endl << std::flush;
            return CR_OK;
        }
        con.color(color);
        int index = 0;
        const char * kit = nyan[index];
        con.gotoxy(1,1);
        //con << "Your DF is now full of kittens!" << std::endl;
        while (kit != 0)
        {
            con.gotoxy(1,1+index);
            con << kit << std::endl;
            index++;
            kit = nyan[index];
        }
        con.flush();
        con.msleep(60);
        ((int&)color) ++;
        if(color > Console::COLOR_MAX)
            color = Console::COLOR_BLUE;
    }
}

#include "dfhack/modules/Creatures.h"
#include "dfhack/VersionInfo.h"
#include <stddef.h>

command_result test_creature_offsets(Core* c, vector< string >& parameters)
{
	uint32_t off_vinfo = c->vinfo->getGroup("Creatures")->getGroup("creature")->/*getGroup("advanced")->*/getOffset("custom_profession");
	uint32_t off_struct = offsetof(df_creature,custom_profession);
    c->con.print("Struct 0x%x, vinfo 0x%x\n", off_struct, off_vinfo);
    return CR_OK;
};

command_result creat_job (Core * c, vector< string >& parameters)
{
    c->Suspend();
    Creatures * cr = c->getCreatures();
    Gui * g = c-> getGui();
    uint32_t num_cr = 0;
    int32_t cx,cy,cz;
    g->getCursorCoords(cx,cy,cz);
    if(cx == -30000)
    {
        c->con.printerr("No cursor.\n");
        c->Resume();
        return CR_FAILURE;
    }
    if(!cr->Start(num_cr) || num_cr == 0)
    {
        c->con.printerr("No creatures.\n");
        c->Resume();
        return CR_FAILURE;
    }
    auto iter = cr->creatures->begin();
    while (iter != cr->creatures->end())
    {
        df_creature * unit = *iter;
        if(cx == unit->x && cy == unit->y && cz == unit->z)
        {
            c->con.print("%d:%s - address 0x%x - job 0x%x\n", unit->id, unit->name.first_name.c_str(), unit, uint32_t(unit) + offsetof(df_creature,current_job));
        }
        iter++;
    }
    c->Resume();
    return CR_OK;
};