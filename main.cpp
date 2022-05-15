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

struct Constraint
{
    int maximum_displacement;
    int minimum_channel_spacing_between_macros;
    int macro_halo;
};
struct Dimension
{
    float width{};
    float height{};
};
struct Component
{
    string macroName;
    string pointType;
    int pointX{};
    int pointY{};
    string orient;
    //lef info
    Dimension size;
};
struct Point
{
    int posX;
    int posY;
};


int main ()
{
    ifstream txt_file (R"(C:\Users\kabazoka\CLionProjects\iccad\2021case3\case3.txt)");
    ifstream lef_file (R"(C:\Users\kabazoka\CLionProjects\iccad\2021case3\case3.lef)");
    ifstream def_file (R"(C:\Users\kabazoka\CLionProjects\iccad\2021case3\case3.lef)");
    ifstream mlist (R"(C:\Users\kabazoka\CLionProjects\iccad\2021case3\case3.def)");
    string in_line;
    vector<Point> die_vector;
    unordered_map<string, Dimension> macro_map; //<macroName, size>
    unordered_map<string, Component> component_map; //<compName, info>
    unordered_map<string, Dimension>::iterator macroIter;
    unordered_map<string, Component>::iterator compIter;
    Constraint constraint;
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
        Dimension macro;
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
            ss1 >> macro.width;
            ss2 >> macro.height;
            macro_map.insert(pair<string, Dimension>(name, macro));
            getline(lef_file, in_line);// line 3 (no meaning)
            getline(lef_file, in_line);// line 4 (no meaning)
        }
    }
    lef_file.close();
    if (def_file.is_open())
    {
        while (getline(def_file, in_line))
        {
            
        }
        
    }
    
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
    else cout << "Unable to open file";

    //test output
    //txt
    cout << "\n***START CONSTRAINT OUTPUT***\n" << endl;
    cout << "powerplan_width_constraint: " << constraint.powerplan_width << endl;
    cout << "minimum_channel_spacing_between_macros_constraint: " << constraint.minimum_channel_spacing_between_macros << endl;
    cout << "buffer_area_reservation_extended_distance: " << constraint.buffer_area_reservation_extended_distance << endl;
    cout << "weight_alpha: " << constraint.weight_alpha << endl;
    cout << "weight_beta: " << constraint.weight_beta << endl;
    cout << "\n***END CONSTRAINT***\n" << endl;
    //lef
    cout << "\n***START LEF OUTPUT***\n" << endl;
    for (macroIter = macro_map.begin(); macroIter != macro_map.end() ; macroIter++)
    {
        cout << macroIter-> first << " / " << macroIter -> second.width << " / " << macroIter -> second.height << endl;
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