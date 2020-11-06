#include "../../include/myhead.h"
#include "../../include/protocol.h"
#include <json/json.h>
int main()
{
    int peerfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in peeraddr;
    bzero(&peeraddr, sizeof(peeraddr));
    peeraddr.sin_family = AF_INET;
    peeraddr.sin_addr.s_addr = INADDR_ANY;
    peeraddr.sin_port = htons(5555);
    int ret = connect(peerfd, (struct sockaddr *)&peeraddr, sizeof(struct sockaddr));
    if (ret == -1)
    {
        perror("connect");
    }
    char tem[65535] = {0};
    while (1)
    {
        bzero(tem, sizeof(tem));
        recv(peerfd, tem, sizeof(tem), 0);
        cout << "recv : " <<endl<< tem << endl;
        string tem_str;
        cout << "------------------------------" << endl;

        cin >> tem_str;

        Json::Value val;
        Json::StyledWriter style_writer;
        val["ID"] = REQUEST_KEY_WORDS;
        val["queryWord"] = tem_str.c_str();
        string sendBuf = style_writer.write(val);
        int size = sendBuf.size();
        send(peerfd, &size, sizeof(size), 0); //小火车 数据长度
        send(peerfd, sendBuf.c_str(), sendBuf.size(), 0);

        //注意 这里需要自己加一个换行符,因为使用cin>>不会将换行符读取进来
    }
}