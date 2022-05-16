#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

using namespace std;

vector<string> splitByPattern(string content, const string& pattern);
string &trim(string &str);
int vector_to_int(vector<int>);

struct Dimension
{
    float width;
    float height;
};
struct Point
{
    int posX;
    int posY;
};
struct Rectangle
{
    Point a;
    Point b;
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
    string site_class;
    Dimension site_size;
};
struct LAYER
{
    string layer_type;
    bool direction;//0 = vertical, 1 = horizontal
    float pitch;
    float width;
};
struct PIN
{
    bool pin_direction;//0 = input;
    LAYER pin_layer;
    vector<Rectangle> rect_vector;
};
struct MACRO_CORE
{
    Dimension size;
    SITE macro_site;
    unordered_map<string, PIN> pin_map;
};
struct MACRO_BLOCK
{
    Dimension size;
    SITE macro_site;
    unordered_map<string, PIN> pin_map;
    unordered_map<LAYER, vector<Rectangle> > obstruction;
};
struct Component
{
    string macroName;
    string pointType;
    int pointX;
    int pointY;
    string orient;
    //lef info
    Dimension size;
};



int main ()
{
    ifstream txt_file (R"(H:\我的雲端硬碟\C++\case01\lefdef\case01.txt)");
    ifstream lef_file (R"(H:\我的雲端硬碟\C++\case01\lefdef\case01.lef)");
    ifstream def_file (R"(C:\Users\kabazoka\CLionProjects\iccad\2021case3\case3.lef)");
    ifstream mlist (R"(C:\Users\kabazoka\CLionProjects\iccad\2021case3\case3.def)");
    string in_line;
    vector<Point> die_vector;
    unordered_map<string, Component> component_map; //<compName, info>
    unordered_map<string, Dimension>::iterator macroIter;
    unordered_map<string, Component>::iterator compIter;
    Constraint constraint;
    //lef info
    int lef_unit;
    float manufacturinggrid;
    SITE site;
    vector<SITE> site_vector;
    unordered_map<string, LAYER> layer_map;
    unordered_map<string, MACRO_CORE> core_macro_map; //<macroName, Macro>
    unordered_map<string, MACRO_BLOCK> block_macro_map; //<macroName, Macro>
    unordered_map<string, MACRO_CORE>::iterator core_macroIter;
    unordered_map<string, MACRO_BLOCK>::iterator block_macroIter;
    //read in txt
    if (txt_file.is_open())
    {
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
    }
    txt_file.close();
    //read in lef
    if (lef_file.is_open())
    {
        Dimension dms;
        vector<string> content_array;
        stringstream ss1, ss2;
        LAYER layer;
        while (getline(lef_file, in_line))
        {
            if (in_line.find("UNITS") != string::npos)
            {
                getline(lef_file, in_line);
                content_array = splitByPattern(in_line, " ");
                ss1 << content_array[2];
                ss1 >> lef_unit;
                getline(lef_file, in_line);//no meaning
            }
            if (in_line.find("MANUFACTURINGFRID") != string::npos)
            {
                content_array = splitByPattern(in_line, " ");
                ss1 << content_array[1];
            }
            if(in_line.find("SITE") != string::npos)
            {
                getline(lef_file, in_line);
                content_array = splitByPattern(in_line, " ");
                ss1 << content_array[1];
                ss2 << content_array[3];
                ss1 >> site.site_size.width;
                ss2 >> site.site_size.height;
                getline(lef_file, in_line);
                content_array = splitByPattern(in_line, " ");
                site.site_class =  content_array[1];
            }
            if(in_line.find("LAYER") != string::npos) //read-in layers
            {
                content_array = splitByPattern(in_line, " ");
                string layer_name = content_array[1];
                getline(lef_file, in_line);
                content_array = splitByPattern(in_line, " ");
                layer.layer_type = content_array[4];
                getline(lef_file, in_line);
                content_array = splitByPattern(in_line, " ");
                if(content_array[1] == "VERTICAL")
                    layer.direction = 0;//0 = vertical, 1 = horizontal
                else
                    layer.direction = 1;
                getline(lef_file, in_line);//PITCH
                content_array = splitByPattern(in_line, " ");
                ss1 << content_array[1];
                ss1 >> layer.pitch;
                getline(lef_file, in_line);//WIDTH
                content_array = splitByPattern(in_line, " ");
                ss1 << content_array[1];
                ss1 >> layer.width;
                layer_map.insert(pair<string, LAYER>(layer_name, layer));
            }
            if(in_line.find("MACRO") != string::npos) //read-in macros
            {
                content_array = splitByPattern(in_line, " ");
                string macro_name = content_array[1];
                getline(lef_file, in_line);
                content_array = splitByPattern(in_line, " ");
                if(content_array[1] == "CORE") //read in core macro
                {
                    PIN pin;
                    MACRO_CORE core_macro;
                    Point pointA, pointB;
                    Rectangle rect;
                    vector<Rectangle> rec_vec;
                    getline(lef_file, in_line);//FOREIGN
                    getline(lef_file, in_line);//ORIGIN
                    getline(lef_file, in_line);//SIZE
                    content_array = splitByPattern(in_line, " ");
                    ss1 << content_array[1];
                    ss2 << content_array[2];
                    ss1 >> core_macro.size.width;
                    ss2 >> core_macro.size.height;
                    getline(lef_file, in_line);//SYMMETRY
                    getline(lef_file, in_line);//SITE
                    getline(lef_file, in_line);//PIN
                    content_array = splitByPattern(in_line, " ");
                    string pin_name = content_array[1];
                    while (getline(lef_file, in_line))//read in pin loop
                    {
                        if (in_line.find("END") != string::npos && in_line.find(pin_name) != string::npos)
                            break;
                        content_array = splitByPattern(in_line, " ");
                        if(content_array[1] == "INPUT")
                            pin.pin_direction = 0;
                        else
                            pin.pin_direction = 1;
                        getline(lef_file, in_line);//PORT
                        getline(lef_file, in_line);//LAYER
                        content_array = splitByPattern(in_line, " ");
                        string layer = content_array[1];
                        pin.pin_layer = layer_map[layer];
                        while(getline(lef_file, in_line))//RECT loop
                        {
                            if (in_line.find("END") != string::npos)
                                break;//end rect loop
                            content_array = splitByPattern(in_line, " ");
                            ss1 << content_array[1];
                            ss2 << content_array[2];
                            ss1 >> pointA.posX;
                            ss2 >> pointA.posY;
                            ss1 << content_array[3];
                            ss2 << content_array[4];
                            ss1 >> pointB.posX;
                            ss2 >> pointB.posY;
                            rect.a = pointA;
                            rect.b = pointB;
                            rec_vec.push_back(rect);
                        }
                    }
                    core_macro_map.insert(pair<string, MACRO_CORE>(macro_name, core_macro));
                    getline(lef_file, in_line); //get the "END MACRO_NAME" line
                }
                else //read-in block macros
                {
                    Dimension dime;
                    MACRO_BLOCK block_macro;
                    PIN pin;
                    Point pointA, pointB;
                    getline(lef_file, in_line); //CLASS
                    getline(lef_file, in_line); //FOREIGN
                    getline(lef_file, in_line); //ORIGIN
                    getline(lef_file, in_line); //SIZE
                    content_array = splitByPattern(in_line, " ");
                    ss1 << content_array[1];
                    ss2 << content_array[2];
                    ss1 >> dime.width;
                    ss2 >> dime.height;
                    block_macro.size = dime;
                    getline(lef_file, in_line); //SYMMETRY
                    getline(lef_file, in_line); 
                    while(in_line.find("PIN") != string::npos) //PIN loop
                    {
                        Rectangle rect;
                        vector<Rectangle> rec_vec;
                        content_array = splitByPattern(in_line, " ");
                        string pin_name = content_array[1];
                        getline(lef_file, in_line); //DIRECTION
                        content_array = splitByPattern(in_line, " ");
                        if(content_array[1] == "INPUT")
                            pin.pin_direction = 0;
                        else
                            pin.pin_direction = 1;
                        getline(lef_file, in_line); //USE SIGNAL
                        getline(lef_file, in_line); //PORT
                        getline(lef_file, in_line); //LAYER
                        content_array = splitByPattern(in_line, " ");
                        string layer_str = content_array[1];
                        pin.pin_layer = layer_map[layer_str];
                        getline(lef_file, in_line); //RECT
                        ss1 << content_array[1];
                        ss2 << content_array[2];
                        ss1 >> pointA.posX;
                        ss2 >> pointA.posY;
                        ss1 << content_array[3];
                        ss2 << content_array[4];
                        ss1 >> pointB.posX;
                        ss2 >> pointB.posY;
                        rect.a = pointA;
                        rect.b = pointB;
                        rec_vec.push_back(rect);
                        getline(lef_file, in_line); //END
                        getline(lef_file, in_line); //END PIN
                        getline(lef_file, in_line); //NEW LINE
                    }
                    if(in_line.find("OBS") != string::npos)
                    {
                        Rectangle rect;
                        vector<Rectangle> rec_vec;
                        getline(lef_file, in_line); //LAYER
                        while (in_line.find("END") != string::npos) //stop reading OBS if "END" occurs
                        {
                            content_array = splitByPattern(in_line, " ");
                            string layer_str = content_array[1];
                            pin.pin_layer = layer_map[layer_str];
                            while(getline(lef_file, in_line))//RECT loop
                            {
                                if (in_line.find("LAYER") != string::npos || in_line.find("END") != string::npos)
                                    break;//end rect loop
                                content_array = splitByPattern(in_line, " ");
                                ss1 << content_array[1];
                                ss2 << content_array[2];
                                ss1 >> pointA.posX;
                                ss2 >> pointA.posY;
                                ss1 << content_array[3];
                                ss2 << content_array[4];
                                ss1 >> pointB.posX;
                                ss2 >> pointB.posY;
                                rect.a = pointA;
                                rect.b = pointB;
                                rec_vec.push_back(rect);
                            }
                        }
                        block_macro.obstruction.insert(pair<LAYER, vector<Rectangle> >(pin.pin_layer, rec_vec));
                    }
                    block_macro_map.insert(pair<string, MACRO_BLOCK>(macro_name, block_macro));
                    getline(lef_file, in_line); //get the "END MACRO_NAME" line
                }
            }
        }
        
        
        /*
        while (getline(lef_file, in_line))// line 1
        {
            if (in_line.find("END") != string::npos)
                break;
            vector<string> lef_content_array = splitByPattern(in_line, " ");
            string name = lef_content_array[1];
            getline(lef_file, in_line);// line 2
            lef_content_array = splitByPattern(in_line, " ");
            stringstream ss1, ss2;
            ss1 << lef_content_array[4];
            ss2 << lef_content_array[6];
            ss1 >> dms.width;
            ss2 >> dms.height;
            macro_map.insert(pair<string, Dimension>(name, dms));
            getline(lef_file, in_line);// line 3 (no meaning)
            getline(lef_file, in_line);// line 4 (no meaning)
        }
        */
    }
    lef_file.close();
    /*
    if (def_file.is_open())
    {
        while (getline(def_file, in_line))
        {
            
        }
        
    }
    */
    /*
    //read in mlist
    if (mlist.is_open())
    {
        while ( getline (mlist, in_line) )
        {
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
                        getline (mlist, in_line);
                    }
                    vector<string> def_content_array = splitByPattern(in_line, " ");
                    if (first_line)
                    {
                        cnt += 1;
                    } else
                        cnt = 9;
                    for (long long unsigned int i = cnt; i < def_content_array.size(); i += 4)
                    {
                        stringstream ss1;
                        stringstream ss2;
                        ss1 << def_content_array[i];
                        ss1 >> die_point.posX;
                        ss2 << def_content_array[i + 1];
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
            //check if readin the COMPONENTS section
            if (in_line.find("COMPONENTS") != string::npos)
            {
                Component component;
                vector<string> def_content_array = splitByPattern(in_line, " ");
                stringstream ss;
                ss << def_content_array[1];
                int compNum;
                ss >> compNum;
                for (int i = 0; i < compNum; i++)
                {
                    int cnt = 3;
                    getline(mlist, in_line);
                    def_content_array = splitByPattern(in_line, " ");
                    string compName = def_content_array[cnt + 1];
                    component.macroName = def_content_array[cnt + 2];
                    component.size = macro_map[def_content_array[cnt + 2]];
                    getline(mlist, in_line);
                    def_content_array = splitByPattern(in_line, " ");
                    cnt = 6;
                    component.pointType = def_content_array[cnt + 1];
                    stringstream ss1;
                    stringstream ss2;
                    ss1 << def_content_array[cnt + 3];
                    ss1 >> component.pointX;
                    ss2 << def_content_array[cnt + 4];
                    ss2 >> component.pointY;
                    component.orient = def_content_array[cnt + 6];
                    component.size = macro_map[component.macroName];
                    component_map.insert(pair<string, Component>(compName, component));
                }
            }
        }
        mlist.close();
        
    }
    else
        cout << "Unable to open mlist file";
    */
    //test output
    //txt
    cout << "\n***START CONSTRAINT OUTPUT***\n" << endl;
    cout << "maximum_displacement_constraint: " << constraint.maximum_displacement << endl;
    cout << "minimum_channel_spacing_between_macros_constraint: " << constraint.minimum_channel_spacing_between_macros << endl;
    cout << "macro_halo: " << constraint.macro_halo << endl;
    cout << "\n***END CONSTRAINT***\n" << endl;
    //lef
    cout << "\n***START LEF OUTPUT***\n" << endl;
    for (core_macroIter = core_macro_map.begin(); core_macroIter != core_macro_map.end() ; macroIter++)
    {
        cout << core_macroIter-> first << " / " << core_macroIter -> second.macro_site.site_class << " / " << core_macroIter -> second.size.width << endl;
    }
    cout << "\n***END LEF***\n" << endl;
    //def
    cout << "\n***START DEF OUTPUT***\n" << endl;
    cout << "DIE POINTS: " << endl;
    for (auto & i : die_vector)
    {
        cout << i.posX << ' ' << i.posY << endl;
    }
    cout << "\nCOMPONENTS: " << endl;
    for (compIter = component_map.begin(); compIter != component_map.end() ; compIter++)
    {
        cout << compIter-> first << " / " << compIter -> second.macroName << " / " << compIter -> second.pointType
             << " / " << compIter -> second.pointX << " / " << compIter -> second.pointY << " / " << compIter -> second.orient
             << " / " << compIter -> second.size.width << " / " << compIter -> second.size.height << endl;
    }
    cout << "\n***END DEF***\n" << endl;
    //end main
    return 0;
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

