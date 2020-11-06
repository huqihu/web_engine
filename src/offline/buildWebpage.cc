#include "../../include/Config.h"
#include "../../include/PageLibPrePrrocess.h"

int main()
{
    Config conf;
    PageLibPreProcess pageProcesser(conf.getConfig("weblib"), conf.getConfig("ripepage.dat"), conf.getConfig("offset.dat"), conf.getConfig("stopWordFile"),conf.getConfig("invertIndexFile"));
    cout << "main 1" << endl;
    pageProcesser.doProcess();
    cout << "main 2" << endl;
    //pageProcesser.showWebPages();
}