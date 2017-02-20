//---------------------------------------------------------------------------

#pragma hdrstop

//---------------------------------------------------------------------------
#include <iostream> //for string
#include <fstream> //for fstream
#include <winsock2.h> //for SOCKET
#include <io.h> //for access
#include "cJSON.h" //for JSON
#include <jpeg.hpp> //for TJPEGImage   // TODO:use libjpeg djpeg get the BMP

using namespace std;

char host[500]; //主机域名
char othPath[800]; //资源地址
SOCKET sock; //建立的socket实例

#define JSON_FILE_NAME "BIN.json"

bool analyUrl(char *url)//仅支持http协议，解析出主机和IP地址
{
    char *pos = strstr(url, "http://");
    if(pos == NULL)
        return false;
    else
        pos +=7;
    sscanf(pos, "%[^/]%s", host, othPath);
    cout<<"host:"<<host<<"   repath:"<<othPath<<endl;
    return true; 
}

void preConnect() //socket进行网络连接
{
    WSADATA wd;
    WSAStartup(MAKEWORD(2, 2), &wd);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == INVALID_SOCKET)
    {
        cout<<"建立socket失败！错误码："<<WSAGetLastError()<<endl;
        return;
    }
    sockaddr_in sa = { AF_INET };
    int n = bind(sock, (sockaddr*)&sa, sizeof(sa));
    if(n == SOCKET_ERROR)
    {
        cout<<"bind函数失败！错误码："<<WSAGetLastError()<<endl;
        return;
    }
    struct hostent *p = gethostbyname(host); //TODO: declare host
    if(p == NULL)
    {
        cout<<"主机无法解析ip!错误码："<<WSAGetLastError()<<endl;
        return; 
    }
    sa.sin_port = htons(80);
    memcpy(&sa.sin_addr, p->h_addr, 4);
    //cout<<*(p->h_addr_list)<<endl;
    n = connect(sock, (sockaddr*)&sa, sizeof(sa));
    if(n == SOCKET_ERROR)
    {
        cout<<"Connect函数失败！错误码："<<WSAGetLastError()<<endl;
        return;
    }
    //向服务器发起GET请求，下载图片
    string reqInfo = "GET "+(string)othPath + " HTTP/1.1\r\nHost: "+(string)host + "\r\nConnection:Close\r\n\r\n";
    if(SOCKET_ERROR == send(sock, reqInfo.c_str(), reqInfo.size(), 0))
    {
        cout<<"send error!错误码："<<WSAGetLastError()<<endl;
        return;
    }
}

void outImage(string imageUrl)
{
    char cImageUrl[800] = {0};
    strcpy(cImageUrl, imageUrl.c_str());
    analyUrl(cImageUrl);
    preConnect();
    string photoname;
    size_t iPos = imageUrl.rfind('/');
    photoname = imageUrl.substr(iPos+1, imageUrl.length()-iPos);
    /*
    photoname.resize(imageUrl.size());
    int k = 0;
    for(int i=0; i<imageUrl.length(); i++)
    {
        char ch = imageUrl[i];
        if( ch!='\\' && ch!=':' && ch!='/'&& ch!='*' && ch!='?' && ch!='"' && ch!='<' && ch!='>' && ch!='|' )
        {
            photoname[k++] = ch;
        }
    }
    photoname = "./img/"+photoname.substr(0, k)+".jpg";*/
    photoname = "img//"+photoname;
    if(access(photoname.c_str(), 0) == 0)
    {
        try
        {
            //Windows的SystemParametersInfo接口对图片图片特别挑剔，一定要BMP格式的，本处则先把jpeg转换为BMP，然后在转回jpeg,这样也能设置成功
            if(photoname.substr(photoname.length()-4,4)==".jpg" );
            {
                Graphics::TBitmap *pBitmap = new Graphics::TBitmap();
                TJPEGImage * aJpeg=new TJPEGImage();
                aJpeg->LoadFromFile(photoname.c_str());
                pBitmap->Assign(aJpeg);

                aJpeg->Assign(pBitmap);
                aJpeg->CompressionQuality = 80;
                aJpeg->SaveToFile(photoname.c_str());
            }
            if(SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (PVOID)photoname.c_str(),
                SPIF_UPDATEINIFILE | SPIF_SENDCHANGE)== false)
            {
                DWORD DWLastError = GetLastError();
                cout << "\nError: "<< DWLastError;
                cout<<"设置桌面背景失败"<<endl;
            }
        }catch(...)
        {
            ;
        }
        return; //如果存在直接返回
    }
    fstream file;
    file.open(photoname.c_str(), ios::out | ios::binary);
    char buf[1024];
    memset(buf, 0, sizeof(buf));
    int n = recv(sock, buf, sizeof(buf)-1, 0);
    char *cpos = strstr(buf, "\r\n\r\n");

    file.write(cpos+strlen("\r\n\r\n"), n- (cpos - buf) - strlen("\r\n\r\n"));
    while( (n = recv(sock, buf, sizeof(buf)-1, 0)) > 0)
    {
        file.write(buf, n);
    }
    file.close();

    try
    {
        SystemParametersInfoA(SPI_SETDESKWALLPAPER, 0, (PVOID)photoname.c_str(),
            SPIF_UPDATEINIFILE | SPIF_SENDCHANGE);
    }catch(...)
    {
        ;
    }
}

void outJson(string jsonUrl)
{
    char cJsonUrl[800] = {0};
    strcpy(cJsonUrl, jsonUrl.c_str());
    analyUrl(cJsonUrl);
    preConnect();
    string jsonname = JSON_FILE_NAME;

    fstream file;
    file.open(jsonname.c_str(), ios::out | ios::binary);
    char buf[2048];
    memset(buf, 0, sizeof(buf));
    int n = recv(sock, buf, sizeof(buf)-1, 0);
    char *cpos = strstr(buf, "\r\n\r\n");

    file.write(cpos+strlen("\r\n\r\n"), n- (cpos - buf) - strlen("\r\n\r\n"));
    while( (n = recv(sock, buf, sizeof(buf)-1, 0)) > 0)
    {
        file.write(buf, n);
    }
    file.close();
}

bool GetImageURLFromJsonFile(string jsonname, char *cOut)
{
    bool bRet = true;
    FILE *f; long len;char *data;
	
	f = fopen(jsonname.c_str(), "rb");fseek(f, 0, SEEK_END); len=ftell(f);fseek(f, 0, SEEK_SET);
	data = (char *)malloc(len+1); fread(data, 1, len, f); fclose(f); //读取文件
	char *out; cJSON *json;cJSON *image;cJSON *url;
	json = cJSON_Parse(data); //解析
    image = cJSON_GetObjectItem(json, "images");
    url = cJSON_GetObjectItem(cJSON_GetArrayItem(image, 0), "url");
    printf("%s", cJSON_Print(url));
    char cTemp[256] = {0};
//    strcpy(cOut, cJSON_Print(url)); //因为存在'"'，不能直接拷贝
    strcpy(cTemp, cJSON_Print(url));

    // remove the '"'
    int k=0;
    for(int i=0; i<=strlen(cTemp); i++)
    {
       if(cTemp[i]!='"')
       {
           cOut[k++] = cTemp[i];
       }
    }
    cOut[k] = '\0';

	if(!json) { printf("Error before: [%s]\n", cJSON_GetErrorPtr()); bRet = false;}
	else
	{
        bRet = false;
	}
	free(data);
    return bRet;
}
#pragma argsused
int main(int argc, char* argv[])
{
    if (access("./img", 0)!=0)
    {
        CreateDirectory("./img", 0);  //在工程下创建文件夹
    }
    string startURL = "http://cn.bing.com/HPImageArchive.aspx?format=js&idx=0&n=1&nc=1439260838289&pid=hp&video=1";
    outJson(startURL); //下载startUrl对应的Json文件

    //获取BIN图片的路径
    char cPath[256] = {0};
    GetImageURLFromJsonFile(JSON_FILE_NAME, cPath);

    string bingImageUrl = "http://"+string(host)+ string(cPath);
    outImage(bingImageUrl.c_str());
//    outImage("http://cn.bing.com/az/hprichbg/rb/Vieste_ZH-CN7832914637_1920x1080.jpg");

    return 0;
}
//---------------------------------------------------------------------------
