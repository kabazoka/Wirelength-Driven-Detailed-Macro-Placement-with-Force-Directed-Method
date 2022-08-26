#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>
#include <utility>
/*
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <sys/types.h>
#include <dirent.h>
#include <cerrno>
*/
using namespace std;


enum Orientation {N = 0, FN = 1, S = 2, FS = 3};
struct Dimension
{
    double width;
    double height;
};
struct Point
{
    double posX;
    double posY;
};
struct Float_Point
{
    double posX;
    double posY;
};
struct Rectangle
{
    Float_Point a;
    Float_Point b;
};

//constraint file struct
struct Constraint
{
    int maximum_displacement;
    int minimum_channel_spacing_between_macros;
    int macro_halo;
};

//lef file struct
struct SITE
{
    string site_name;
    string site_class;
    Dimension site_size{};
};
struct LAYER
{
    string layer_type;
    bool direction{};//0 = vertical, 1 = horizontal
    double pitch{};
    double width{};
};
struct PIN
{
    bool pin_direction{};//0 = input;
    string pin_name;
    LAYER pin_layer;
    vector<Rectangle> rect_vector;
    string connected_wire;
    Float_Point relativePoint{};
};
struct STD_CELL_LEF
{
    Dimension size{};
    SITE macro_site;
    unordered_map<string, PIN> pin_map;
};
struct MACRO_LEF
{
    Dimension size{};
    SITE macro_site;
    unordered_map<string, PIN> pin_map;
    unordered_map<string, vector<Rectangle> > obstruction;
};
//Def file and mlist file struct
struct STD_CELL
{
    string macroType;
    string cellName;
    string placeType;
    double posX;
    double posY;
    string orient;
    //lef info
    Dimension size{};
    SITE macro_site;
    unordered_map<string, PIN> pin_map;
};
struct MACRO
{
    string macroType;
    string macroName;
    string placeType;
    double posX{};
    double posY{};
    Point originPos;
    string orient;
    //lef info
    Dimension size{};
    SITE macro_site;
    unordered_map<string, PIN> pin_map;
    unordered_map<string, vector<Rectangle> > obstruction;
};
//ver
struct PIN_INDEX
{
    string pin_name;
    string macro_name;
};
struct FORCE
{
    double xForce{};
    double yForce{};
};
struct FORCE_INFO
{
    FORCE force;
    string macroName;
    FORCE remainForce;
};

//variables
Constraint constraint{};
string in_line;
vector<Point> die_vector;
unordered_map<string, MACRO>::iterator compIter;
//lef info
int lef_unit;
double manufacturinggrid;
unordered_map<string, SITE> site_map;
unordered_map<string, LAYER> layer_map;
unordered_map<string, STD_CELL_LEF> cell_class_map; //<macroType, Macro>
unordered_map<string, MACRO_LEF> macro_class_map; //<macroType, Macro>
//def info
unordered_map<string, MACRO> macro_map; //<compName, info>
unordered_map<string, STD_CELL> cell_map; //<compName, info>
unordered_map<string, STD_CELL_LEF>::iterator core_class_iter;
unordered_map<string, MACRO_LEF>::iterator block_class_iter;
unordered_map<string, STD_CELL>::iterator cellIter;
unordered_map<string, MACRO>::iterator macroIter1;
//mList
int macroCount = 0;

//verilog info
unordered_map<string, vector<PIN_INDEX>> netlistMap; //<wire name, vector<pin/macro>>
unordered_map<string, vector<PIN_INDEX>>::iterator netIter;

unordered_map<string, PIN>::iterator pin_map_iter;
unordered_map<string, MACRO> mod_macro_map; //modified macro map

//functions
//int getdir(string dir, vector<string> &files);

double getDistance(Point, Point);//return the manhattan distance
bool checkOverlap(const MACRO&, unordered_map<string, MACRO>&);//return true if overlapped
bool doOverlap(Float_Point, Float_Point, Float_Point, Float_Point);
vector<string> splitByPattern(string content, const string& pattern);
string &trim(string &str);
int vector_to_int(vector<int>);
double getWireLength(unordered_map<string, MACRO>&, unordered_map<string, STD_CELL>&);//per macro
MACRO macroDisplace(MACRO, int);
bool checkOutOfBounds(const MACRO&);//return true if out of bounds

void read_constraint(const string& fileName)
{
    ifstream txt_file;
    txt_file.open(fileName);
    if (txt_file.is_open())
    {
        cout << "# Reading constraints..." << endl;
        for (int i = 0; i < 3; ++i)
        {
            getline(txt_file, in_line);
            vector<string> content_array = splitByPattern(in_line, " ");
            stringstream ss;
            ss << content_array[1];
            if (i == 0)
            {
                ss >> constraint.maximum_displacement;
            }
            else if (i == 1)
            {
                ss >> constraint.minimum_channel_spacing_between_macros;
            }
            else if (i == 2)
            {
                ss >> constraint.macro_halo;
            }
        }
        cout << "# Constraints reading completed.\n" << endl;
        txt_file.close();
    }
    else
        cout << "# Unable to open Constraint txt file. \n" << endl;
}

void read_lef_file(const string& fileName)
{
    ifstream lef_file;
    lef_file.open(fileName);
    if (lef_file.is_open())
    {
        cout << "# Reading LEF file..." << endl;
        vector<string> content_array;
        stringstream ss1, ss2;
        while (getline(lef_file, in_line))
        {
            if (in_line.find("UNITS") != string::npos)
            {
                ss1 = {}, ss2 = {};
                getline(lef_file, in_line);
                content_array = splitByPattern(in_line, " ");
                ss1 << content_array[2];
                ss1 >> lef_unit;
                getline(lef_file, in_line);//no meaning
            }
            if (in_line.find("MANUFACTURINGFRID") != string::npos)
            {
                ss1 = {}, ss2 = {};
                content_array = splitByPattern(in_line, " ");
                ss1 << content_array[1];
                ss1 >> manufacturinggrid;
            }
            if(in_line.find("SITE") != string::npos)
            {
                ss1 = {}, ss2 = {};
                SITE site;
                string site_name;
                content_array = splitByPattern(in_line, " ");
                site_name = content_array[1];
                getline(lef_file, in_line);
                content_array = splitByPattern(in_line, " ");
                ss1 << content_array[1];
                ss2 << content_array[3];
                ss1 >> site.site_size.width;
                ss2 >> site.site_size.height;
                getline(lef_file, in_line);
                content_array = splitByPattern(in_line, " ");
                site.site_class =  content_array[1];
                site_map.insert(pair<string, SITE>(site_name, site));
                getline(lef_file, in_line); //end site
            }
            if(in_line.find("LAYER") != string::npos) //read-in layers
            {
                LAYER layer;
                ss1 = {}, ss2 = {};
                content_array = splitByPattern(in_line, " ");
                string layer_name = content_array[1];
                getline(lef_file, in_line);
                content_array = splitByPattern(in_line, " ");
                layer.layer_type = content_array[1];
                getline(lef_file, in_line);
                content_array = splitByPattern(in_line, " ");
                layer.direction = !(content_array[1] == "VERTICAL");
                getline(lef_file, in_line);//PITCH
                content_array = splitByPattern(in_line, " ");
                ss1 << content_array[1];
                ss1 >> layer.pitch;
                getline(lef_file, in_line);//WIDTH
                content_array = splitByPattern(in_line, " ");
                ss2 << content_array[1];
                ss2 >> layer.width;
                layer_map.insert(pair<string, LAYER>(layer_name, layer));
                getline(lef_file, in_line); //end layer
            }

            if(in_line.find("MACRO") != string::npos) //read-in macros
            {
                unordered_map<string, PIN> pin_map;
                content_array = splitByPattern(in_line, " ");
                string macro_name = content_array[1];
                getline(lef_file, in_line);
                content_array = splitByPattern(in_line, " ");
                if(content_array[1] == "CORE") //read in core macro
                {
                    PIN pin;
                    STD_CELL_LEF core_macro;
                    Float_Point pointA{}, pointB{};
                    Rectangle rect{};
                    ss1 = {}, ss2 = {};
                    vector<Rectangle> rec_vec;
                    getline(lef_file, in_line);//FOREIGN
                    getline(lef_file, in_line);//ORIGIN
                    getline(lef_file, in_line);//SIZE
                    content_array = splitByPattern(in_line, " ");
                    ss1 << content_array[1];
                    ss2 << content_array[3];
                    ss1 >> core_macro.size.width;
                    ss2 >> core_macro.size.height;
                    getline(lef_file, in_line); //SYMMETRY
                    getline(lef_file, in_line); //SITE
                    content_array = splitByPattern(in_line, " ");
                    string site_name = content_array[1];
                    core_macro.macro_site = site_map[site_name];
                    while (getline(lef_file, in_line))//read in pin loop
                    {
                        if (in_line.find("END") != string::npos)
                            break;
                        content_array = splitByPattern(in_line, " ");
                        string pinName = content_array[1];
                        pin.pin_name = pinName;
                        content_array = splitByPattern(in_line, " ");
                        getline(lef_file, in_line); //DIRECTION
                        pin.pin_direction = !(content_array[1] == "INPUT");
                        getline(lef_file, in_line); //PORT
                        if (in_line.find("USE"))
                            getline(lef_file, in_line);
                        getline(lef_file, in_line); //LAYER
                        content_array = splitByPattern(in_line, " ");
                        string layer_str = content_array[1];
                        pin.pin_layer = layer_map[layer_str];
                        while(getline(lef_file, in_line)) //RECT loop
                        {
                            ss1 = {}, ss2 = {};
                            if (in_line.find("END") != string::npos)
                                break;//end rect loop
                            content_array = splitByPattern(in_line, " ");
                            ss1 << content_array[1];
                            ss2 << content_array[2];
                            ss1 >> pointA.posX;
                            ss2 >> pointA.posY;
                            ss1 = {}, ss2 = {};
                            ss1 << content_array[3];
                            ss2 << content_array[4];
                            ss1 >> pointB.posX;
                            ss2 >> pointB.posY;
                            rect.a = pointA;
                            rect.b = pointB;
                            rec_vec.push_back(rect);
                            //cal
                            if (pin.relativePoint.posX == 0 && pin.relativePoint.posY == 0)
                            {
                                pin.relativePoint.posX = (pointA.posX + pointB.posX) / 2;
                                pin.relativePoint.posY = (pointA.posY + pointB.posY) / 2;
                            }
                            else
                            {
                                double tempX, tempY;
                                tempX = (pointA.posX + pointB.posX) / 2;
                                tempY = (pointA.posY + pointB.posY) / 2;
                                pin.relativePoint.posX = (tempX + pin.relativePoint.posX) / 2;
                                pin.relativePoint.posY = (tempY + pin.relativePoint.posY) / 2;
                            }
                        }
                        pin.rect_vector = rec_vec;
                        pin_map.insert(pair<string, PIN>(pinName, pin));
                        getline(lef_file, in_line); //end pin
                    }
                    core_macro.pin_map = pin_map;
                    cell_class_map.insert(pair<string, STD_CELL_LEF>(macro_name, core_macro));
                    getline(lef_file, in_line); //get the "END MACRO_NAME" line
                }
                else //read-in block macros
                {
                    ss1 = {}, ss2 = {};
                    Dimension dime{};
                    MACRO_LEF block_macro;
                    PIN pin;
                    Float_Point pointA{}, pointB{};
                    getline(lef_file, in_line); //CLASS
                    getline(lef_file, in_line); //FOREIGN
                    getline(lef_file, in_line); //ORIGIN
                    content_array = splitByPattern(in_line, " ");
                    ss1 << content_array[1];
                    ss2 << content_array[3];
                    ss1 >> dime.width;
                    ss2 >> dime.height;
                    block_macro.size = dime;
                    getline(lef_file, in_line); //SIZE
                    getline(lef_file, in_line); //SYMMETRY
                    while(in_line.find("PIN") != string::npos) //PIN loop
                    {
                        ss1 = {}, ss2 = {};
                        Rectangle rect{};
                        vector<Rectangle> rec_vec;
                        content_array = splitByPattern(in_line, " ");
                        string pinName = content_array[1];
                        pin.pin_name = pinName;
                        getline(lef_file, in_line); //DIRECTION
                        content_array = splitByPattern(in_line, " ");
                        if(content_array[1] == "INPUT")
                            pin.pin_direction = false;
                        else
                            pin.pin_direction = true;
                        getline(lef_file, in_line); //USE SIGNAL
                        getline(lef_file, in_line); //PORT
                        getline(lef_file, in_line); //LAYER
                        content_array = splitByPattern(in_line, " ");
                        string layer_str = content_array[1];
                        pin.pin_layer = layer_map[layer_str];
                        while(getline(lef_file, in_line)) //RECT loop
                        {
                            ss1 = {}, ss2 = {};
                            if (in_line.find("END") != string::npos)
                                break;//end rect loop
                            content_array = splitByPattern(in_line, " ");
                            ss1 << content_array[1];
                            ss2 << content_array[2];
                            ss1 >> pointA.posX;
                            ss2 >> pointA.posY;
                            ss1 = {}, ss2 = {};
                            ss1 << content_array[3];
                            ss2 << content_array[4];
                            ss1 >> pointB.posX;
                            ss2 >> pointB.posY;
                            rect.a = pointA;
                            rect.b = pointB;
                            rec_vec.push_back(rect);
                            //cal
                            if (pin.relativePoint.posX == 0 && pin.relativePoint.posY == 0)
                            {
                                pin.relativePoint.posX = (pointA.posX + pointB.posX) / 2;
                                pin.relativePoint.posY = (pointA.posY + pointB.posY) / 2;
                            }
                            else
                            {
                                double tempX, tempY;
                                tempX = (pointA.posX + pointB.posX) / 2;
                                tempY = (pointA.posY + pointB.posY) / 2;
                                pin.relativePoint.posX = (tempX + pin.relativePoint.posX) / 2;
                                pin.relativePoint.posY = (tempY + pin.relativePoint.posY) / 2;
                            }
                        }
                        pin.rect_vector = rec_vec;
                        pin_map.insert(pair<string, PIN>(pinName, pin));
                        getline(lef_file, in_line); //END
                        getline(lef_file, in_line); //END PIN
                    }
                    block_macro.pin_map = pin_map;
                    if(in_line.find("OBS") != string::npos)
                    {
                        Rectangle rect{};
                        string layer_str;
                        vector<Rectangle> rec_vec;
                        getline(lef_file, in_line); //LAYER
                        while (in_line.find("END") != string::npos) //stop reading OBS if "END" occurs
                        {
                            content_array = splitByPattern(in_line, " ");
                            layer_str = content_array[1];
                            pin.pin_layer = layer_map[layer_str];
                            while(getline(lef_file, in_line))//RECT loop
                            {
                                ss1 = {}, ss2 = {};
                                if (in_line.find("LAYER") != string::npos || in_line.find("END") != string::npos)
                                    break;//end rect loop
                                content_array = splitByPattern(in_line, " ");
                                ss1 << content_array[1];
                                ss2 << content_array[2];
                                ss1 >> pointA.posX;
                                ss2 >> pointA.posY;
                                ss1 = {}, ss2 = {};
                                ss1 << content_array[3];
                                ss2 << content_array[4];
                                ss1 >> pointB.posX;
                                ss2 >> pointB.posY;
                                rect.a = pointA;
                                rect.b = pointB;
                                rec_vec.push_back(rect);
                            }
                        }
                        block_macro.obstruction.insert(pair<string, vector<Rectangle> >(layer_str, rec_vec));
                    }
                    macro_class_map.insert(pair<string, MACRO_LEF>(macro_name, block_macro));
                    getline(lef_file, in_line); //get the "END MACRO_NAME" line
                }
            }
        }
        cout << "# LEF file reading completed.\n" << endl;
        lef_file.close();
    }
    else
        cout << "# Unable to open LEF file.\n" << endl;
}

void read_def_file(const string& fileName)
{
    ifstream def_file;
    def_file.open(fileName);
    if (def_file.is_open())
    {
        cout << "# Reading DEF file..." << endl;
        while(getline(def_file, in_line))
        {
            if (in_line.find("DIEAREA") != string::npos)
            {
                bool first_line = true;
                bool last_line = true;
                int cnt = 1;
                Point die_point{};
                while (last_line)
                {
                    if (!first_line)
                    {
                        getline (def_file, in_line);
                    }
                    vector<string> content_array = splitByPattern(in_line, " ");
                    if (first_line)
                    {
                        cnt += 1;
                    } else
                        cnt = 0;
                    for (long long unsigned int i = cnt; i < content_array.size(); i += 4)
                    {
                        stringstream ss1;
                        stringstream ss2;
                        ss1 << content_array[i];
                        ss1 >> die_point.posX;
                        ss2 << content_array[i + 1];
                        ss2 >> die_point.posY;
                        die_vector.push_back(die_point);
                    }
                    first_line = false;
                    if (in_line.find(';') != string::npos)
                    {
                        last_line = false;
                    }
                }
            }

            if (in_line.find("ROW"))
            {
                vector<string> content_array = splitByPattern(in_line, " ");
            }

            if (in_line.find("COMPONENTS") != string::npos)
            {
                STD_CELL stdCell;
                STD_CELL_LEF stdCellLef;
                vector<string> content_array = splitByPattern(in_line, " ");
                stringstream ss;
                int compNum;
                ss << content_array[1];
                ss >> compNum;
                for (int i = 0; i < compNum; i++)
                {
                    getline(def_file, in_line);
                    content_array = splitByPattern(in_line, " ");
                    string compName = content_array[1];
                    stdCell.cellName = compName;
                    stdCell.macroType = content_array[2];
                    stdCellLef = cell_class_map[content_array[2]];
                    stdCell.size = stdCellLef.size;
                    stdCell.macro_site = stdCellLef.macro_site;
                    stdCell.pin_map = stdCellLef.pin_map;
                    getline(def_file, in_line);
                    content_array = splitByPattern(in_line, " ");
                    stdCell.placeType = content_array[1];
                    stringstream ss1;
                    stringstream ss2;
                    ss1 << content_array[3];
                    ss1 >> stdCell.posX;
                    ss2 << content_array[4];
                    ss2 >> stdCell.posY;
                    stdCell.orient = content_array[6];
                    cell_map.insert(pair<string, STD_CELL>(compName, stdCell));
                }
            }
        }
        cout << "# DEF file reading completed.\n" << endl;
    }
    else
        cout << "# Unable to open def file.\n" << endl;
}

int macroCountMlist = 0;
void read_mlist_file(const string& fileName)
{
    ifstream mlist_file;
    mlist_file.open(fileName);
    if (mlist_file.is_open())
    {
        cout << "# Reading mlist file..." << endl;
        while ( getline (mlist_file, in_line) )
        {
            /*
            //check if readin the DIEAREA section
            if (in_line.find("DIEAREA") != string::npos)
            {
                bool first_line = true;
                bool last_line = true;
                int cnt = 1;
                Point die_point{};
                while (last_line)
                {
                    if (!first_line)
                    {
                        getline (mlist_file, in_line);
                    }
                    vector<string> content_array = splitByPattern(in_line, " ");
                    if (first_line)
                    {
                        cnt += 1;
                    } else
                        cnt = 0;
                    for (long long unsigned int i = cnt; i < content_array.size(); i += 4)
                    {
                        stringstream ss1;
                        stringstream ss2;
                        ss1 << content_array[i];
                        ss1 >> die_point.posX;
                        ss2 << content_array[i + 1];
                        ss2 >> die_point.posY;
                        die_vector.push_back(die_point);
                    }
                    first_line = false;
                    if (in_line.find(';') != string::npos)
                    {
                        last_line = false;
                    }
                }
            }
             */
            //check if readin the COMPONENTS section
            if (in_line.find("COMPONENTS") != string::npos)
            {
                MACRO macro;
                MACRO_LEF macroLef;
                vector<string> content_array = splitByPattern(in_line, " ");
                stringstream ss;
                int compNum;
                ss << content_array[1];
                ss >> compNum;
                macroCountMlist = compNum;
                macroCount = compNum;
                for (int i = 0; i < compNum; i++)
                {
                    getline(mlist_file, in_line);
                    content_array = splitByPattern(in_line, " ");
                    string compName = content_array[1];
                    macro.macroName = compName;
                    macro.macroType = content_array[2];
                    macroLef = macro_class_map[content_array[2]];
                    macro.size = macroLef.size;
                    macro.macro_site = macroLef.macro_site;
                    macro.pin_map = macroLef.pin_map;
                    getline(mlist_file, in_line);
                    content_array = splitByPattern(in_line, " ");
                    macro.placeType = content_array[1];
                    stringstream ss1;
                    stringstream ss2;
                    ss1 << content_array[3];
                    ss1 >> macro.posX;
                    ss2 << content_array[4];
                    ss2 >> macro.posY;
                    macro.orient = content_array[6];
                    macro_map.insert(pair<string, MACRO>(compName, macro));
                    cell_class_map.erase(macro.macroType);
                    cell_map.erase(macro.macroName);
                }
            }
        }
        cout << "# mlist file reading completed.\n" << endl;
        mlist_file.close();
    }
    else
        cout << "# Unable to open mlist file\n\n";
}

void read_verilog_file(const string& fileName)
{
    ifstream verilog_file;
    verilog_file.open(fileName);
    vector<string> content_array;
    if (verilog_file.is_open())
    {
        cout << "# Reading verilog..." << endl;
        while( getline (verilog_file, in_line))
        {
            if(in_line.find("cells") != string::npos)
            {
                while(getline (verilog_file, in_line))
                {
                    if (in_line.empty())
                        break;
                    content_array = splitByPattern(in_line, " ");
                    auto item = cell_class_map.find(content_array[0]);
                    if (item != cell_class_map.end()) // cell
                    {
                        STD_CELL cell;
                        unordered_map<string, PIN> pin_map;
                        PIN local_pin;
                        vector<string> con_arr;
                        string str1, str2, wire_name;
                        PIN_INDEX pinIndex;
                        vector<PIN_INDEX> basicPinVec;

                        cell = cell_map[content_array[1]];
                        pin_map = cell.pin_map;
                        int line_length = content_array.size();
                        line_length -= 4;
                        for (int i = 0; i < line_length; ++i) {
                            str1 = content_array[3 + i];
                            con_arr = splitByPattern(str1, "(");
                            str1 = con_arr[1]; //wire name
                            str2 = con_arr[0]; //pin name
                            str2.erase(0,1);
                            local_pin = pin_map[str2];
                            con_arr = splitByPattern(str1, ")");
                            wire_name = con_arr[0];
                            local_pin.connected_wire = wire_name;
                            pin_map[str2] = local_pin;
                            //
                            basicPinVec = netlistMap[wire_name];
                            netlistMap.erase(wire_name);
                            pinIndex.macro_name = cell.cellName;
                            pinIndex.pin_name = str2;
                            basicPinVec.push_back(pinIndex);
                            netlistMap.insert(pair<string, vector<PIN_INDEX>>(wire_name, basicPinVec));
                        }
                        cell.pin_map = pin_map;
                        cell_map[content_array[1]] = cell;
                    }
                    else // macro
                    {
                        MACRO macro;
                        unordered_map<string, PIN> pin_map;
                        PIN local_pin;
                        vector<string> con_arr;
                        string str1, str2, wire_name;
                        PIN_INDEX pinIndex;
                        vector<PIN_INDEX> basicPinVec;

                        macro = macro_map[content_array[1]];
                        pin_map = macro.pin_map;
                        int line_length = content_array.size();
                        line_length -= 4;
                        for (int i = 0; i < line_length; ++i) {
                            str1 = content_array[3 + i];
                            con_arr = splitByPattern(str1, "(");
                            str1 = con_arr[1];
                            str2 = con_arr[0]; //pin name
                            str2.erase(0,1);
                            local_pin = pin_map[str2];
                            con_arr = splitByPattern(str1, ")");
                            wire_name = con_arr[0];
                            local_pin.connected_wire = wire_name;
                            pin_map[str2] = local_pin;
                            //
                            basicPinVec = netlistMap[wire_name];
                            netlistMap.erase(wire_name);
                            pinIndex.macro_name = macro.macroName;
                            pinIndex.pin_name = str2;
                            basicPinVec.push_back(pinIndex);
                            netlistMap.insert(pair<string, vector<PIN_INDEX>>(wire_name, basicPinVec));
                        }
                        macro.pin_map = pin_map;
                        macro_map[content_array[1]] = macro;
                    }
                }
            }
        }
        cout << "# Verilog file reading completed.\n" << endl;
    }
}

void flipping(unordered_map<string, MACRO> macroMap, unordered_map<string, STD_CELL> cellMap)
{
    cout << "# Start macro flipping..." << endl;
    double shortestSum = 0, initialSum = 0;
    for (macroIter1 = macroMap.begin(); macroIter1 != macroMap.end() ; macroIter1++) //loop of all macros
    {
        if (macroIter1->second.placeType == "FIXED")
            continue;
        double shortestLength = 0, initialLength = 0;
        MACRO sourceMacro = macroIter1 -> second;
        STD_CELL targetCell;
        unordered_map<string, MACRO>::iterator macroIter2;
        unordered_map<string, STD_CELL>::iterator cellIter2;
        Orientation currentOri = N, shortestOri = N;
        if (sourceMacro.orient == "N")
            currentOri = N;
        else if(sourceMacro.orient == "FN")
            currentOri = FN;
        else if(sourceMacro.orient == "S")
            currentOri = S;
        else if(sourceMacro.orient == "FS")
            currentOri = FS;
        for (int ori = currentOri; ori <= 3; ori++)//flipping the macro
        {
            double sumOfLength = 0;
            for (pin_map_iter = sourceMacro.pin_map.begin(); pin_map_iter != sourceMacro.pin_map.end() ; pin_map_iter++) //loop of all pins in a macro
            {
                Point sourceP{}, targetP{};
                PIN sourcePin = pin_map_iter->second;
                string wireName = sourcePin.connected_wire; //get the pin's connected wire
                vector<PIN_INDEX> pinVec = netlistMap[wireName];
                for (auto & i : pinVec) //loop of all of the pins that the wire connects
                {
                    PIN_INDEX pinIndex = i;
                    double tmpX{}, tmpY{};
                    tmpX = (sourceMacro.posX) / 2000;
                    tmpY = (sourceMacro.posY) / 2000;
                    if (ori == 0) //N
                    {
                        sourceP.posX = tmpX + sourcePin.relativePoint.posX;
                        sourceP.posY = tmpY + sourcePin.relativePoint.posY;
                    }
                    if (ori == 1) //FN
                    {
                        sourceP.posX = tmpX + (sourceMacro.size.width - sourcePin.relativePoint.posX);
                        sourceP.posY = tmpY + sourcePin.relativePoint.posY;
                    }
                    if (ori == 2) //S
                    {
                        sourceP.posX = tmpX + sourcePin.relativePoint.posX;
                        sourceP.posY = tmpY + (sourceMacro.size.height - sourcePin.relativePoint.posY);
                    }
                    if (ori == 3) //FS
                    {
                        sourceP.posX = tmpX + (sourceMacro.size.width - sourcePin.relativePoint.posX);
                        sourceP.posY = tmpY + (sourceMacro.size.height - sourcePin.relativePoint.posY);
                    }
                    auto item = cellMap.find(pinIndex.macro_name);
                    if (item != cellMap.end())
                    {
                        cellIter2 = cellMap.find(pinIndex.macro_name); //find the target cell
                        targetCell = cellIter2->second;
                        unordered_map<string, PIN> targetPinMap = targetCell.pin_map; //get the target pin map
                        PIN targetPin = targetPinMap[pinIndex.pin_name]; //get the pin from the pin map with index
                        tmpX = (targetCell.posX) / 2000;
                        tmpY = (targetCell.posY) / 2000;
                        targetP.posX = tmpX + targetPin.relativePoint.posX;
                        targetP.posY = tmpY + targetPin.relativePoint.posY;
                    }
                    else
                    {
                        macroIter2 = macroMap.find(pinIndex.macro_name); //find the target macro
                        MACRO targetMacro = macroIter2->second;
                        unordered_map<string, PIN> targetPinMap = targetMacro.pin_map; //get the target pin map
                        PIN targetPin = targetPinMap[pinIndex.pin_name]; //get the pin from the pin map with index
                        tmpX = (targetMacro.posX) / 2000;
                        tmpY = (targetMacro.posY) / 2000;
                        targetP.posX = tmpX + targetPin.relativePoint.posX;
                        targetP.posY = tmpY + targetPin.relativePoint.posY;
                    }
                    sumOfLength += getDistance(sourceP, targetP);
                }
            }
            currentOri = static_cast<Orientation>(ori);
            if (shortestLength == 0 || shortestLength > sumOfLength)
            {
                shortestLength = sumOfLength;
                shortestOri = currentOri;
            }
            if (initialLength == 0)
            {
                initialLength = sumOfLength;
            }
        }
        if (shortestOri == N)
            sourceMacro.orient = "N";
        else if(shortestOri == FN)
            sourceMacro.orient = "FN";
        else if(shortestOri == S)
            sourceMacro.orient = "S";
        else if(shortestOri == FS)
            sourceMacro.orient = "FS";
        macro_map[sourceMacro.macroName] = sourceMacro;
        shortestSum += shortestLength;
        initialSum += initialLength;
    }
    double total = initialSum - shortestSum;
    total /= initialSum;
    total *=100;
    cout << "# Macro flipping completed." << endl;
    cout << "# Reduced " << total << "% of wire length. \n" << endl;
}
/*
void flipping2(unordered_map<string, MACRO> macroMap, unordered_map<string, STD_CELL> cellMap)
{
    cout << "# Start macro flipping again..." << endl;
    multimap<string, MACRO> multiMacroMap;
    multimap<string, MACRO>:: iterator multiMacroMapIter;
    for (macroIter1 = macroMap.begin(); macroIter1 != macroMap.end() ; macroIter1++) //loop of all macros
    {
        multiMacroMap.insert(pair<string, MACRO>(macroIter1->first, macroIter1->second));
    }
    double shortestSum = 0, initialSum = 0;
    for (multiMacroMapIter = multiMacroMap.begin(); multiMacroMapIter != multiMacroMap.end() ; multiMacroMapIter++) //loop of all macros
    {
        if (multiMacroMapIter->second.placeType == "FIXED")
            continue;
        double shortestLength = 0, initialLength = 0;
        MACRO sourceMacro = multiMacroMapIter -> second;
        STD_CELL targetCell;
        unordered_map<string, MACRO>::iterator macroIter2;
        unordered_map<string, STD_CELL>::iterator cellIter2;
        Orientation currentOri = N, shortestOri = N;
        if (sourceMacro.orient == "N")
            currentOri = N;
        else if(sourceMacro.orient == "FN")
            currentOri = FN;
        else if(sourceMacro.orient == "S")
            currentOri = S;
        else if(sourceMacro.orient == "FS")
            currentOri = FS;
        for (int ori = currentOri; ori <= 3; ori++)//flipping the macro
        {
            double sumOfLength = 0;
            for (pin_map_iter = sourceMacro.pin_map.begin(); pin_map_iter != sourceMacro.pin_map.end() ; pin_map_iter++) //loop of all pins in a macro
            {
                Point sourceP{}, targetP{};
                PIN sourcePin = pin_map_iter->second;
                string wireName = sourcePin.connected_wire; //get the pin's connected wire
                vector<PIN_INDEX> pinVec = netlistMap[wireName];
                for (auto & i : pinVec) //loop of all of the pins that the wire connects
                {
                    PIN_INDEX pinIndex = i;
                    double tmpX{}, tmpY{};
                    tmpX = (sourceMacro.posX) / 2000;
                    tmpY = (sourceMacro.posY) / 2000;
                    if (ori == 0) //N
                    {
                        sourceP.posX = tmpX + sourcePin.relativePoint.posX;
                        sourceP.posY = tmpY + sourcePin.relativePoint.posY;
                    }
                    if (ori == 1) //FN
                    {
                        sourceP.posX = tmpX + (sourceMacro.size.width - sourcePin.relativePoint.posX);
                        sourceP.posY = tmpY + sourcePin.relativePoint.posY;
                    }
                    if (ori == 2) //S
                    {
                        sourceP.posX = tmpX + sourcePin.relativePoint.posX;
                        sourceP.posY = tmpY + (sourceMacro.size.height - sourcePin.relativePoint.posY);
                    }
                    if (ori == 3) //FS
                    {
                        sourceP.posX = tmpX + (sourceMacro.size.width - sourcePin.relativePoint.posX);
                        sourceP.posY = tmpY + (sourceMacro.size.height - sourcePin.relativePoint.posY);
                    }
                    auto item = cellMap.find(pinIndex.macro_name);
                    if (item != cellMap.end())
                    {
                        cellIter2 = cellMap.find(pinIndex.macro_name); //find the target cell
                        targetCell = cellIter2->second;
                        unordered_map<string, PIN> targetPinMap = targetCell.pin_map; //get the target pin map
                        PIN targetPin = targetPinMap[pinIndex.pin_name]; //get the pin from the pin map with index
                        tmpX = (targetCell.posX) / 2000;
                        tmpY = (targetCell.posY) / 2000;
                        targetP.posX = tmpX + targetPin.relativePoint.posX;
                        targetP.posY = tmpY + targetPin.relativePoint.posY;
                    }
                    else
                    {
                        macroIter2 = macroMap.find(pinIndex.macro_name); //find the target macro
                        MACRO targetMacro = macroIter2->second;
                        unordered_map<string, PIN> targetPinMap = targetMacro.pin_map; //get the target pin map
                        PIN targetPin = targetPinMap[pinIndex.pin_name]; //get the pin from the pin map with index
                        tmpX = (targetMacro.posX) / 2000;
                        tmpY = (targetMacro.posY) / 2000;
                        targetP.posX = tmpX + targetPin.relativePoint.posX;
                        targetP.posY = tmpY + targetPin.relativePoint.posY;
                    }
                    sumOfLength += getDistance(sourceP, targetP);
                }
            }
            currentOri = static_cast<Orientation>(ori);
            if (shortestLength == 0 || shortestLength > sumOfLength)
            {
                shortestLength = sumOfLength;
                shortestOri = currentOri;
            }
            if (initialLength == 0)
            {
                initialLength = sumOfLength;
            }
        }
        if (shortestOri == N)
            sourceMacro.orient = "N";
        else if(shortestOri == FN)
            sourceMacro.orient = "FN";
        else if(shortestOri == S)
            sourceMacro.orient = "S";
        else if(shortestOri == FS)
            sourceMacro.orient = "FS";
        macro_map[sourceMacro.macroName] = sourceMacro;
        shortestSum += shortestLength;
        initialSum += initialLength;
    }
    double total = initialSum - shortestSum;
    total /= initialSum;
    total *=100;
    cout << "# Macro flipping completed." << endl;
    cout << "# Reduced " << total << "% of wire length. \n" << endl;
}
*/
void displace(unordered_map<string, MACRO>& macroMap, const unordered_map<string, STD_CELL>& cellMap)
{
    cout << "# Start macro displacement..." << endl;
    multimap<double, FORCE_INFO> forceMap;
    for (macroIter1 = macroMap.begin(); macroIter1 != macroMap.end() ; macroIter1++) //loop of all macros
    {
        if (macroIter1->second.placeType == "FIXED")
            continue;
        //start calculating where to displace
        macroIter1->second.originPos.posX = macroIter1->second.posX;
        macroIter1->second.originPos.posY = macroIter1->second.posY;
        MACRO sourceMacro = macroIter1 -> second;
        STD_CELL targetCell;
        unordered_map<string, MACRO>::iterator macroIter2;
        unordered_map<string, STD_CELL>::iterator cellIter2;
        double xForce{}, yForce{}, magnitude{};
        FORCE force;
        FORCE_INFO forceInfo;
        for (pin_map_iter = sourceMacro.pin_map.begin(); pin_map_iter != sourceMacro.pin_map.end() ; pin_map_iter++) //loop of all pins in a macro
        {
            Point sourceP{}, targetP{};
            double x_force{}, y_force{};
            FORCE F;
            PIN sourcePin = pin_map_iter->second;
            string wireName = sourcePin.connected_wire; //get the pin's connected wire
            vector<PIN_INDEX> pinVec = netlistMap[wireName];
            //cout << "source macro: " << sourceMacro.macroName << " source pin " << sourcePin.pin_name << endl;
            for (auto & i : pinVec) //loop of all the pins that the wire connects
            {
                PIN_INDEX pinIndex = i;
                if (sourceMacro.macroName == pinIndex.macro_name)
                    continue;
                double tmpX{}, tmpY{};
                tmpX = sourceMacro.posX / 2000;
                tmpY = sourceMacro.posY / 2000;
                if (sourceMacro.orient == "N") //N
                {
                    sourceP.posX = tmpX + sourcePin.relativePoint.posX;
                    sourceP.posY = tmpY + sourcePin.relativePoint.posY;
                }
                if (sourceMacro.orient == "FN") //FN
                {
                    sourceP.posX = tmpX + (sourceMacro.size.width - sourcePin.relativePoint.posX);
                    sourceP.posY = tmpY + sourcePin.relativePoint.posY;
                }
                if (sourceMacro.orient == "S") //S
                {
                    sourceP.posX = tmpX + sourcePin.relativePoint.posX;
                    sourceP.posY = tmpY + (sourceMacro.size.height - sourcePin.relativePoint.posY);
                }
                if (sourceMacro.orient == "FS") //FS
                {
                    sourceP.posX = tmpX + (sourceMacro.size.width - sourcePin.relativePoint.posX);
                    sourceP.posY = tmpY + (sourceMacro.size.height - sourcePin.relativePoint.posY);
                }
                auto item = cellMap.find(pinIndex.macro_name);
                if (item != cellMap.end())
                {
                    cellIter2 = cell_map.find(pinIndex.macro_name); //find the target cell
                    targetCell = cellIter2->second;
                    targetP.posX = (targetCell.posX) / 2000;
                    targetP.posY = (targetCell.posY) / 2000;
                    //cout << "target cell: " << cellIter2->second.cellName << " " << cellIter2->second.posX / 2000 <<
                    //" " << cellIter2->second.posY / 2000<< endl;
                }
                else
                {
                    macroIter2 = macro_map.find(pinIndex.macro_name); //find the target macro
                    MACRO targetMacro = macroIter2->second;
                    unordered_map<string, PIN> targetPinMap = targetMacro.pin_map; //get the target pin map
                    PIN targetPin = targetPinMap[pinIndex.pin_name]; //get the pin from the pin map with index
                    tmpX = (targetMacro.posX) / 2000;
                    tmpY = (targetMacro.posY) / 2000;
                    targetP.posX = tmpX + targetPin.relativePoint.posX;
                    targetP.posY = tmpY + targetPin.relativePoint.posY;
                    //cout << "target macro: " << macroIter2->second.macroName << " " << macroIter2->second.posX / 2000
                    //<< " " << macroIter2->second.posY / 2000<< endl;
                }
                //force
                F.xForce = targetP.posX - sourceP.posX;
                F.yForce = targetP.posY - sourceP.posY;
                //cout << "sourcePos: " << sourceP.posX << " " << sourceP.posY << " targetPos: " << targetP.posX << " " << targetP.posY << endl;
                //cout << F.xForce << " " << F.yForce << endl;
                double bevel = sqrt(F.xForce*F.xForce + F.yForce*F.yForce);
                F.xForce = F.xForce / bevel;
                F.yForce = F.yForce / bevel;
                x_force += F.xForce;
                y_force += F.yForce;
            }
            xForce += x_force;
            yForce += y_force;
            magnitude = abs(xForce) + abs(yForce);
        }
        xForce = constraint.maximum_displacement * (xForce / magnitude);
        yForce = constraint.maximum_displacement * (yForce / magnitude);
        //cout << sourceMacro.macroName << " " << xForce << " " << yForce << endl;
        force.xForce = xForce;
        force.yForce = yForce;
        if (force.xForce < 0 && force.yForce < 0)
        {
            if (abs(force.xForce - ceil(force.xForce)) < abs(force.yForce - ceil(force.yForce)))
            {
                force.xForce = ceil(force.xForce);
                force.yForce = floor(force.yForce);
            }
            else
            {
                force.xForce = floor(force.xForce);
                force.yForce = ceil(force.yForce);
            }
        }
        else if(force.xForce > 0 && force.yForce < 0)
        {
            if (force.xForce - floor(force.xForce) > abs(force.yForce - ceil(force.yForce)))
            {
                force.xForce = ceil(force.xForce);
                force.yForce = ceil(force.yForce);
            }
            else
            {
                force.xForce = floor(force.xForce);
                force.yForce = floor(force.yForce);
            }
        }
        else if(force.xForce < 0 && force.yForce > 0)
        {
            if (abs(force.xForce - ceil(force.xForce)) < force.yForce - floor(force.yForce))
            {
                force.xForce = ceil(force.xForce);
                force.yForce = ceil(force.yForce);
            }
            else
            {
                force.xForce = floor(force.xForce);
                force.yForce = floor(force.yForce);
            }
        }
        else
        {
            if (force.xForce - floor(force.xForce) < force.yForce - floor(force.yForce))
            {
                force.xForce = floor(force.xForce);
                force.yForce = ceil(force.yForce);
            }
            else
            {
                force.xForce = ceil(force.xForce);
                force.yForce = floor(force.yForce);
            }
        }
        forceInfo.force = force;
        forceInfo.macroName = macroIter1->second.macroName;
        cout << sourceMacro.macroName << " " << xForce << " " << yForce << " / " << magnitude << endl;
        forceMap.insert(pair<double, FORCE_INFO>(magnitude, forceInfo));
    }
    cout << "# Finished force calculation. Displacing..." << endl;
    //***finished calculation and start displacement***
    int overlapCount = 0, boundCount = 0;
    //cout << "# Displacement run#1..." << endl;
    FORCE remainForce, nextRoundForce;
    for (auto& i : forceMap)
    {
        //bool xValidMove = true, yValidMove = true;
        MACRO& sourceMacro = macroMap[i.second.macroName];
        MACRO cloneMacro = sourceMacro;
        //cout << "# Displacing " << sourceMacro.macroName << endl;
        remainForce.xForce = i.second.force.xForce;
        remainForce.yForce = i.second.force.yForce;
        while(remainForce.xForce != 0 || remainForce.yForce != 0)
        {
            if (remainForce.xForce != 0) {//movement on x-axis
                if (i.second.force.xForce > 0)
                    cloneMacro = macroDisplace(cloneMacro, 1);
                else
                    cloneMacro = macroDisplace(cloneMacro, 3);
                if (checkOverlap(cloneMacro, macroMap))
                {
                    nextRoundForce.xForce = remainForce.xForce;
                    remainForce.xForce = 0;
                    overlapCount++;
                    cloneMacro.posX = sourceMacro.posX;
                }
                else if (checkOutOfBounds(cloneMacro))
                {
                    nextRoundForce.xForce = remainForce.xForce;
                    remainForce.xForce = 0;
                    boundCount++;
                    cloneMacro.posX = sourceMacro.posX;
                }
                else
                {
                    macro_map[cloneMacro.macroName].posX = cloneMacro.posX;
                    if (remainForce.xForce > 0)
                        remainForce.xForce -= 1;
                    else if (remainForce.xForce < 0)
                        remainForce.xForce += 1;
                }
            }
            if (remainForce.yForce != 0)
            {
                //movement on y-axis
                if (remainForce.yForce > 0)
                    cloneMacro = macroDisplace(cloneMacro, 0);
                else
                    cloneMacro = macroDisplace(cloneMacro, 2);
                if (checkOverlap(cloneMacro, macroMap))
                {
                    nextRoundForce.yForce = remainForce.yForce;
                    remainForce.yForce = 0;
                    overlapCount++;
                    cloneMacro.posY = sourceMacro.posY;
                }
                else if (checkOutOfBounds(cloneMacro))
                {
                    nextRoundForce.yForce = remainForce.yForce;
                    remainForce.yForce = 0;
                    boundCount++;
                    cloneMacro.posY = sourceMacro.posY;
                }
                else
                {
                    macro_map[cloneMacro.macroName].posY = cloneMacro.posY;
                    if (remainForce.yForce > 0)
                        remainForce.yForce -= 1;
                    else if (remainForce.yForce < 0)
                        remainForce.yForce += 1;
                }
            }
        }
    }
    cout << "# Overlap occurred " << overlapCount << " times." << endl;
    cout << "# Out of bounds occurred " << boundCount << " times." << endl;
    cout << "# Displacement completed.\n" << endl;
}

void calculateDisplaceDistance(unordered_map<string, MACRO>& macroMap)
{
    Point p1{}, p2{};
    for (macroIter1 = macroMap.begin(); macroIter1 != macroMap.end() ; macroIter1++) //loop of all macros
    {
        p1.posX = macroIter1->second.posX;
        p1.posY = macroIter1->second.posY;
        p2.posX = macroIter1->second.originPos.posX;
        p2.posY = macroIter1->second.originPos.posY;
        double dis = getDistance(p1, p2);
        cout << macroIter1->second.macroName << " displaced " << dis << endl;
    }
}

void output(string filename)
{
    ofstream ofs;
    string str1, str2;
    stringstream ss1, ss2;
    ss1 << filename[4];
    ss1 >> str1;
    ss2 << filename[5];
    ss2 >> str2;
    str1 = str1 + str2;
    ofs.open(filename);
    if (!ofs.is_open())
    {
        cout << "# Failed to open output DMP file.\n";
    }
    else
    {
        cout << "# Printing DMP file...\n" << endl;
        ofs << "VERSION 5.7 ;\n";
        ofs << "DESIGN case" << str1 << " ;\n" ;
        ofs << "UNITS DISTANCE MICRONS 2000 ;\n\n";
        ofs << "DIEAREA ";
        for (auto p : die_vector)
        {
            ofs << "( " << (int)p.posX << " " << (int)p.posY << " ) ";
        }
        ofs << ";\n\n";
        ofs << "COMPONENTS " << macro_map.size() << " ;\n";
        for (const auto& macro : macro_map)
        {
            ofs << "   - " << macro.second.macroName << " " << macro.second.macroType << endl;
            ofs << "      + PLACED ( " << (int)macro.second.posX << " " << (int)macro.second.posY << " ) "
                << macro.second.orient << " ;" << endl;
        }
        ofs << "END COMPONENTS\n\n\n\n";
        ofs << "END DESIGN\n\n";
        ofs.close();
        cout << "# DMP printing completed.\n" << endl;
    }
}

int main(int argc, char* argv[])
{
    // ./DMP caseOO.v caseOO.lef caseOO.def caseOO.mlist caseOO.txt caseOO.dmp
    /*
    read_constraint("case01.txt");
    read_lef_file("case01.lef");
    read_def_file("case01.def");
    read_mlist_file("case01.mlist");
    read_verilog_file("case01.v");

    flipping(macro_map, cell_map);

    output("case01.dmp");
     */

    /*
    double wireLengthBeforeDisplace = getWireLength(macro_map, cell_map);
    cout << "# Before displace: " << wireLengthBeforeDisplace << endl;
    displace(macro_map, cell_map);
    double wireLengthAfterDisplace = getWireLength(macro_map, cell_map);
    cout << "# After displace: " << wireLengthAfterDisplace << endl;
    cout << "# After displacement: " << wireLengthAfterDisplace / wireLengthBeforeDisplace * 100
    << "% of original wire length."<< endl;


    calculateDisplaceDistance(macro_map);
    flipping(macro_map, cell_map);
    */

    string current_exec_name = argv[0]; // Name of the current exec program

    vector<string> all_args(argv, argv + argc);

    for (const auto& i : all_args)
    {
        cout << i << endl;
    }
    read_constraint(all_args[5]);
    read_lef_file(all_args[2]);
    read_def_file(all_args[3]);
    read_mlist_file(all_args[4]);
    read_verilog_file(all_args[1]);

    flipping(macro_map, cell_map);
    output(all_args[6]);

    /*
    char path_buffer[_MAX_PATH];
    char drive[_MAX_DRIVE];
    char dir[_MAX_DIR];
    char fname[_MAX_FNAME];
    char ext[_MAX_EXT];
    _makepath( path_buffer, "D", "\\C++\\case01\\lefdef\\", "case01", "txt" );
    cout << endl;
    printf( "Path created with _makepath: %s\n\n", path_buffer );
    _splitpath( path_buffer, drive, dir, fname, ext );
    printf( "Path extracted with _splitpath:\n" );
    printf( "  Drive: %s\n", drive );
    printf( "  Dir: %s\n", dir );
    printf( "  Filename: %s\n", fname );
    printf( "  Ext: %s\n", ext );

    string dirc = string(".");//資料夾路徑(絕對位址or相對位址)
    vector<string> files = vector<string>();
    getdir(dirc, files);
    //輸出資料夾和檔案名稱於螢幕
    for(auto & file : files){
        cout << file << endl;
    }
    system("pause");
     */

    //test output
    //txt
    /*
    cout << "\n***START CONSTRAINT OUTPUT***\n" << endl;
    cout << "maximum_displacement_constraint: " << constraint.maximum_displacement << endl;
    cout << "minimum_channel_spacing_between_macros_constraint: " << constraint.minimum_channel_spacing_between_macros << endl;
    cout << "macro_halo: " << constraint.macro_halo << endl;
    cout << "\n***END CONSTRAINT***\n" << endl;
     */
    //lef
    /*
    cout << "\n***START LEF OUTPUT***\n" << endl;
    for (core_class_iter = cell_class_map.begin(); core_class_iter != cell_class_map.end() ; core_class_iter++)
    {
        cout << core_class_iter-> first << " / " << core_class_iter -> second.macro_site.site_class << " / " << core_class_iter -> second.size.width << " / " << core_class_iter -> second.size.height << endl;
    }
    cout << "\n***FINISHED CELLS***\n" << endl;
    for (block_class_iter = macro_class_map.begin(); block_class_iter != macro_class_map.end() ; block_class_iter++)
    {
        cout << block_class_iter-> first << " / " << block_class_iter -> second.macro_site.site_class << " / " << block_class_iter -> second.size.width << " / " << block_class_iter -> second.size.height<< endl;
    }
    cout << "\n***END LEF***\n" << endl;
     */

    /*
     //def
    cout << "\n***START DEF OUTPUT***\n" << endl;
    cout << "DIE POINTS: " << endl;
    for (auto & i : die_vector)
    {
        cout << i.posX << ' ' << i.posY << endl;
    }
    cout << "\nCOMPONENTS: " << endl;
    for (cellIter = cell_map.begin(); cellIter != cell_map.end() ; cellIter++)
    {
        cout << cellIter-> first << " / " << cellIter -> second.macroType << " / " << cellIter -> second.placeType
             << " / " << cellIter -> second.posX << " / " << cellIter -> second.posY << " / " << cellIter -> second.orient
             << " / " << cellIter -> second.size.width << " / " << cellIter -> second.size.height << endl;
    }
    cout << "\n***END DEF***\n" << endl;
     */

    //mlist
    /*
    cout << "\n***START MLIST OUTPUT***\n" << endl;
    cout << "DIE POINTS: " << endl;
    for (auto & i : die_vector)
    {
        cout << i.posX << ' ' << i.posY << endl;
    }
    cout << "\nCOMPONENTS: " << endl;
    for (compIter = macro_map.begin(); compIter != macro_map.end() ; compIter++)
    {
        cout << compIter-> first << " / " << compIter -> second.macroType << " / " << compIter -> second.placeType
             << " / " << compIter -> second.posX << " / " << compIter -> second.posY << " / " << compIter -> second.orient
             << " / " << compIter -> second.size.width << " / " << compIter -> second.size.height << endl;
    }
    cout << "\n***END MLIST***\n" << endl;
     */

    //verilog
    /*
    for (netIter = netlistMap.begin(); netIter != netlistMap.end(); netIter++)
    {
        cout << netIter->first << endl;
    }
     */


    //end main

    return 0;
}
/*
int getdir(string dir, vector<string> &files)
{
    DIR *dp;//創立資料夾指標
    struct dirent *dirp;
    if((dp = opendir(dir.c_str())) == NULL){
        cout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }
    while((dirp = readdir(dp)) != NULL){//如果dirent指標非空
        files.push_back(string(dirp->d_name));//將資料夾和檔案名放入vector
    }
    closedir(dp);//關閉資料夾指標
    return 0;
}
*/
double getDistance(Point A, Point B)
{
    double distance = abs(A.posX - B.posX) + abs(A.posY - B.posY);
    return distance;
}

//splitting string by pattern
vector<string> splitByPattern(string content, const string& pattern)
{
    vector<string> words;
    size_t pos = 0;
    trim(content);
    while ((pos = content.find(pattern)) != string::npos)
    {
        string word = content.substr(0, pos);
        words.push_back(trim(word));
        content.erase(0, pos + pattern.length());
    }
    words.push_back(trim(content));
    return words;
}

//remove the blank spaces of the front and rear ends
string &trim(string &str)
{
    if (str.empty())
    {
        return str;
    }
    int str_length;
    int start = 0;
    int space_end = str.find_first_not_of(" ");
    str.erase(start, space_end);
    int space_start = (str.find_last_not_of(" ") + 1);
    str_length = str.length();
    str.erase(space_start, str_length);

    return str;
}

//converting the vector to int
int vector_to_int(vector<int> num)
{
    long long n = 0;
    int N = num.size();
    for (int i = 0; i < N; i++) {
        n += num[i]*pow(10, N-i-1);
    }
    return n;
}

double getWireLength(unordered_map<string, MACRO>& macroMap, unordered_map<string, STD_CELL>& cellMap)//per macro
{
    double sumOfLength = 0;
    for (macroIter1 = macroMap.begin(); macroIter1 != macroMap.end() ; macroIter1++) //loop of all macros
    {
        double shortestLength = 0, initialLength = 0;
        MACRO sourceMacro = macroIter1 -> second;
        STD_CELL targetCell;
        unordered_map<string, MACRO>::iterator macroIter2;
        unordered_map<string, STD_CELL>::iterator cellIter2;
        for (pin_map_iter = sourceMacro.pin_map.begin(); pin_map_iter != sourceMacro.pin_map.end() ; pin_map_iter++) //loop of all pins in a macro
        {
            Point sourceP{}, targetP{};
            PIN sourcePin = pin_map_iter->second;
            string wireName = sourcePin.connected_wire; //get the pin's connected wire
            vector<PIN_INDEX> pinVec = netlistMap[wireName];
            for (auto & i : pinVec) //loop of all of the pins that the wire connects
            {
                PIN_INDEX pinIndex = i;
                double tmpX{}, tmpY{};
                tmpX = (sourceMacro.posX) / 2000;
                tmpY = (sourceMacro.posY) / 2000;
                sourceP.posX = tmpX + sourcePin.relativePoint.posX;
                sourceP.posY = tmpY + sourcePin.relativePoint.posY;
                auto item = cellMap.find(pinIndex.macro_name);
                if (item != cellMap.end())
                {
                    cellIter2 = cellMap.find(pinIndex.macro_name); //find the target macro
                    targetCell = cellIter2->second;
                    unordered_map<string, PIN> targetPinMap = targetCell.pin_map; //get the target pin map
                    PIN targetPin = targetPinMap[pinIndex.pin_name]; //get the pin from the pin map with index
                    tmpX = (targetCell.posX) / 2000;
                    tmpY = (targetCell.posY) / 2000;
                    targetP.posX = tmpX + targetPin.relativePoint.posX;
                    targetP.posY = tmpY + targetPin.relativePoint.posY;
                }
                else
                {
                    macroIter2 = macro_map.find(pinIndex.macro_name); //find the target macro
                    MACRO targetMacro = macroIter2->second;
                    unordered_map<string, PIN> targetPinMap = targetMacro.pin_map; //get the target pin map
                    PIN targetPin = targetPinMap[pinIndex.pin_name]; //get the pin from the pin map with index
                    tmpX = (targetMacro.posX) / 2000;
                    tmpY = (targetMacro.posY) / 2000;
                    targetP.posX = tmpX + targetPin.relativePoint.posX;
                    targetP.posY = tmpY + targetPin.relativePoint.posY;
                }
                sumOfLength += getDistance(sourceP, targetP);
            }
        }
    }
    return sumOfLength;
}

MACRO macroDisplace(MACRO macro, int direction)
{
    double dpu = 2000.0;
    if (direction == 0)//up
    {
        macro.posY += dpu;
    }
    else if (direction == 1)//right
    {
        macro.posX += dpu;
    }
    else if (direction == 2)//down
    {
        macro.posY -= dpu;
    }
    else if (direction == 3)//left
    {
        macro.posX -= dpu;
    }

    return macro;
}


bool checkOverlap(const MACRO& macro, unordered_map<string, MACRO>& macroMap)//return true if overlapped
{
    unordered_map<string, MACRO>::iterator macroIter2;
    bool overlapped = false;
    Float_Point l1{}, r1{}, l2{}, r2{};
    l1.posX = macro.posX;
    l1.posY = macro.posY + macro.size.height * 2000;
    r1.posX = macro.posX + macro.size.width * 2000;
    r1.posY = macro.posY;
    int counter = 0;
    for (macroIter2 = macroMap.begin(); macroIter2 != macroMap.end() ; macroIter2++) //loop of all macros
    {
        //counter++;
        if (macro.macroName == macroIter2->second.macroName)//skip the same component
            continue;
        l2.posX = macroIter2->second.posX;
        l2.posY = macroIter2->second.posY + macroIter2->second.size.height * 2000;
        r2.posX = macroIter2->second.posX + macroIter2->second.size.width * 2000;
        r2.posY = macroIter2->second.posY;
        if(doOverlap(l1, r1, l2, r2))
        {
            overlapped = true;
            cout << "# " << macro.macroName << " and " << macroIter2->second.macroName << " overlapped." << endl;
            break;
        }
    }
    //cout << "Looped through " << counter << " macros in overlap detection." << endl;
    if (overlapped)
    {
        return true;
    }
    else
        return false;
}

// Returns true if two rectangles (l1, r1) and (l2, r2)
// overlap
bool doOverlap(Float_Point l1, Float_Point r1, Float_Point l2, Float_Point r2)
{
    // if rectangle has area 0, no overlap
    if (l1.posX == r1.posX || l1.posY == r1.posY || r2.posX == l2.posX || l2.posY == r2.posY)
        return false;
    // If one rectangle is on left side of other
    if (l1.posX > r2.posX || l2.posX > r1.posX)
        return false;
    // If one rectangle is above other
    if (r1.posY > l2.posY || r2.posY > l1.posY)
        return false;

    return true;
}

bool checkOutOfBounds(const MACRO& macro)//return true if out of bounds
{
    if (macro.posX + macro.size.width*2000 > die_vector[1].posX)
    {
        cout << macro.macroName << " out of bounds on x-axis!" << endl;
        return true;
    }
    if (macro.posY + macro.size.height*2000 > die_vector[1].posY)
    {
        cout << macro.macroName << " out of bounds on y-axis!" << endl;
        return true;
    }


    return false;
}
