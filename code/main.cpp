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

char host[500]; //��������
char othPath[800]; //��Դ��ַ
SOCKET sock; //������socketʵ��

#define JSON_FILE_NAME "BIN.json"

bool analyUrl(char *url)//��֧��httpЭ�飬������������IP��ַ
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

void preConnect() //socket������������
{
    WSADATA wd;
    WSAStartup(MAKEWORD(2, 2), &wd);
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == INVALID_SOCKET)
    {
        cout<<"����socketʧ�ܣ������룺"<<WSAGetLastError()<<endl;
        return;
    }
    sockaddr_in sa = { AF_INET };
    int n = bind(sock, (sockaddr*)&sa, sizeof(sa));
    if(n == SOCKET_ERROR)
    {
        cout<<"bind����ʧ�ܣ������룺"<<WSAGetLastError()<<endl;
        return;
    }
    struct hostent *p = gethostbyname(host); //TODO: declare host
    if(p == NULL)
    {
        cout<<"�����޷�����ip!�����룺"<<WSAGetLastError()<<endl;
        return; 
    }
    sa.sin_port = htons(80);
    memcpy(&sa.sin_addr, p->h_addr, 4);
    //cout<<*(p->h_addr_list)<<endl;
    n = connect(sock, (sockaddr*)&sa, sizeof(sa));
    if(n == SOCKET_ERROR)
    {
        cout<<"Connect����ʧ�ܣ������룺"<<WSAGetLastError()<<endl;
        return;
    }
    //�����������GET��������ͼƬ
    string reqInfo = "GET "+(string)othPath + " HTTP/1.1\r\nHost: "+(string)host + "\r\nConnection:Close\r\n\r\n";
    if(SOCKET_ERROR == send(sock, reqInfo.c_str(), reqInfo.size(), 0))
    {
        cout<<"send error!�����룺"<<WSAGetLastError()<<endl;
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
            //Windows��SystemParametersInfo�ӿڶ�ͼƬͼƬ�ر����ޣ�һ��ҪBMP��ʽ�ģ��������Ȱ�jpegת��ΪBMP��Ȼ����ת��jpeg,����Ҳ�����óɹ�
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
                cout<<"�������汳��ʧ��"<<endl;
            }
        }catch(...)
        {
            ;
        }
        return; //�������ֱ�ӷ���
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
	data = (char *)malloc(len+1); fread(data, 1, len, f); fclose(f); //��ȡ�ļ�
	char *out; cJSON *json;cJSON *image;cJSON *url;
	json = cJSON_Parse(data); //����
    image = cJSON_GetObjectItem(json, "images");
    url = cJSON_GetObjectItem(cJSON_GetArrayItem(image, 0), "url");
    printf("%s", cJSON_Print(url));
    char cTemp[256] = {0};
//    strcpy(cOut, cJSON_Print(url)); //��Ϊ����'"'������ֱ�ӿ���
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
        CreateDirectory("./img", 0);  //�ڹ����´����ļ���
    }
    string startURL = "http://cn.bing.com/HPImageArchive.aspx?format=js&idx=0&n=1&nc=1439260838289&pid=hp&video=1";
    outJson(startURL); //����startUrl��Ӧ��Json�ļ�

    //��ȡBINͼƬ��·��
    char cPath[256] = {0};
    GetImageURLFromJsonFile(JSON_FILE_NAME, cPath);

    string bingImageUrl = "http://"+string(host)+ string(cPath);
    outImage(bingImageUrl.c_str());
//    outImage("http://cn.bing.com/az/hprichbg/rb/Vieste_ZH-CN7832914637_1920x1080.jpg");

    return 0;
}
//---------------------------------------------------------------------------
