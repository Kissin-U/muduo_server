#include "json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <vector>
#include <map>
#include <string>
using namespace std;

// json序列化示例一
void func1()
{
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["msg"] = "hey, what r u doing?";

    string sendBuf = js.dump(); // dump就是把{"from":"zhang san","msg":"hey, what r u doing?","msg_type":2,"to":"li si"}赋值给sendBuf


    // printf("%s", sendBuf.c_Str());
    //cout << sendBuf << endl; // 这样就可以了 直接输出字符串
    // 复习，cout printf只能打印字符，因此得强制转换，put可以输出字符串；

    puts(sendBuf.c_str()); // puts必须得转换一下
    // 最后排出来有点像无序哈希表，他并不是msg——type最开始插入就是在最开始
}

void func2()
{
json js;
// 添加数组 键可以是整型，也可以是字符串，也可以是数组
js["id"] = {1, 2, 3, 4, 5};
// 添加key-value
js["name"] = "zhang san";
// 添加对象
js["msg"]["zhang san"] = "hello world"; // 这是在 "msg" 对象中创建一个新的键 "zhang san"，并将其值设置为 "hello world"。
js["msg"]["liu shuo"] = "hello china";
// 上下等同，一次性添加数组对象
js["msg"] = {{"zhang san", "hello world"}, {"liu shuo", "hello china"}}; // 写了两遍，相当于给上面的覆盖了
cout << js << endl;
}
// json序列化代码3，容器序列化
void func3()
{
    json js;

    // 直接序列化一个vector容器
    vector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(5);

    js["list"] = vec;

    // 直接序列化一个map容器
    map<int, string> m;
    m.insert({1, "黄山"});
    m.insert({2, "华山"});
    m.insert({3, "泰山"});
    
    js["path"] = m;

    // cout << js << endl;

    // 也是可以发出去

    string sendBuf = js.dump(); // json数据对象 =》 序列化 json字符串 

    cout << sendBuf << endl;

}

int main()
{
    // func1();
    // func2();
    func3();
    return 0;

    

}