#include "json.hpp"
using json = nlohmann::json;

#include <iostream>
using namespace std;

void func(){
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhan san";
    js["to"] = "li si";
    js["msg"] = "hello, what are you doing?";
    cout<<js<<endl;
}

// int main(){
//     json js;
//     vector<int> vec;
//     vec.push_back(1);
//     vec.push_back(2);
//     vec.push_back(3);
//     js["list"] = vec;

//     map<int, string> m;
//     m.insert({1, "huang"});
//     m.insert({2, "zhang"});
//     js["path"] = m;
//     // cout<<js<<endl;
//     string jsonStr = js.dump();
//     cout<<jsonStr<<endl;

//     json js2 = json::parse(jsonStr);
//     cout<<js2<<endl;

//     string name = js2["name"];
//     cout<<"name: "<<name<<endl;

//     vector<int> v=js["list"];
//     for(int val : v){
//         cout<<val<<endl;
//     }
//     return 0;
// }