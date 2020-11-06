#include "../../include/tcpServer_v4.h"
#include "../../include/Mylogger.h"
#include "../../include/Config.h"
#include "../../include/PageLibPrePrrocess.h"
#include "../../include/MyWeb.h"
//#include "webTask.h"
int main()
{
    Config conf;
    MyWeb web(conf.getConfig("ripepage.dat"), conf.getConfig("offset.dat"), conf.getConfig("stopWordFile"), conf.getConfig("invertIndexFile"));
    web.loadInvertIndex();
    //web.searchWebPages("取得成功");
    //web.searchWebPages("这个时代");
    //web.searchWebPages("青少年是语言变化的真正推动者和促进者");
    web.searchWebPages("体育竞赛");
    //web.searchWebPages("在里约奥运会男子游泳400米决赛之前");
    //web.searchWebPages("理解");
    string line;
    while (getline(cin, line))
    {
        web.searchWebPages(line);
        cout<<"搜索网页 "<<endl;
    }
}