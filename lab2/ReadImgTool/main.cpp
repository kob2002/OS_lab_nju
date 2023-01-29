#include <iostream>
#include <string>
#include "vector"
#include "Tool.h"
#include <fstream>
#include <sstream>
using namespace std;

//******************** 与nasm联合的部分
/*
extern "C" {
void asm_print(const char *, const int);
}

*/
/**
 * 输出到控制台的函数
 * @param s
 *//*

void myPrint(string s) {
    //把string转化为char*
    char* str=new char [s.length()+1];
    int i=0;
    for(;i<s.length();i++) str[i]=s[i];
    str[i]='\0';
    asm_print(str, s.length());
}
*/

//****************** end

//文件夹（目录）节点 有子结点(包括存放于数据区的子目录的目录项)
class FileNode;
string transformCharArrToBinaryStr(int startIndex);
int transformBinaryToDecimal(int startIndex,int endIndex);
char fileContent[2879*512]={0}; //FAT共2879个cluster，每个512字节  从左到右正向存
bool isDir(string clusterBinaryStr);
/**
 * 把二进制串转为十进制
 * @param str
 * @return
 */
int transformBinaryToInt(string str){
    int ret=1;
    int result=0;
    for(int i=str.length()-1;i>=0;i--){
        if(str[i]=='1'){
            result+=ret;
        }
        ret*=2;
    }
    return result;
}
/**
 * 将小端存储的字符串转换为十进制数
 * @param str  长度仅限于8的倍数
 * @return
 */
int smallEndTransToInt(string str){
    int byteNum=str.length()/8;
    string buffer="";
    int answer=0;
    for(int i=0;i<byteNum;i++){
        string temp=str.substr(i*8,8);
        buffer=temp+buffer;
    }
    answer= transformBinaryToInt(buffer);
    return answer;
}

/**
 * 把二进制字符串转换为对应的ASCII下的字符串 (不会把不可见的字符放进来)
 * @param bStr
 * @return
 */
string transBinaryStrToASCII(string bStr){
    string answer="";
    for(int i=0;i<bStr.length()/8;i++){
        string temp=bStr.substr(i*8,8);
        int t= transformBinaryToInt(temp);
        if(t==32) continue;
        //cout<<"t="<<t<<" ";
        answer+=(char )t;
    }
    //cout<<endl;
    return answer;
}

/**
 * 把二进制字符串转换为可理解的名字（可以是文件夹也可以是文件）
 * @param name 二进制串
 * @return
 */
string solveName(string name){
    string ASCStr= transBinaryStrToASCII(name);
    string answer="";
    for(int i=0;i<ASCStr.length();i++){
        if(ASCStr[i]!=' ') answer+=ASCStr[i];
    }
    return ASCStr;
}
/**
 * 该类是索引条目的类，不是文件夹节点
 */
class DirectoryEntry{

private:
    //注意，里面都是二进制的字符串
    string fileName="";//8byte
    string extensionName="";//3byte
    string attribute="";//1byte
    string firstClusterStr="";//2byte 首簇的低16位，FAT的首簇高16位常为0
    int firstClusterNum=0;//存储位置的首扇区号
    int lengthOfFile=0;//文件长度
    bool isDirectory= true; //该索引是文件夹的还是文件的
public:
    /**
     *
     * @param fileStr 一个32byte的目录项 二进制字符串（长度=32*8）
     */
    DirectoryEntry(string fileStr){
        this->fileName= solveName(fileStr.substr(0,8*8));
        this->extensionName= solveName(fileStr.substr(8*8,3*8));
        this->attribute=fileStr.substr(11*8,1*8);
        this->firstClusterStr=fileStr.substr(26*8,2*8);
        this->firstClusterNum= smallEndTransToInt(this->firstClusterStr);
        this->lengthOfFile= smallEndTransToInt(fileStr.substr(28*8,4*8));
        this->isDirectory= isDir(fileStr);
    }

    int getFATNum(){
        return this->firstClusterNum;
    }
    void setFile(){
        this->isDirectory= false;
    }
    /**
     * 获得该条目是否是文件的索引还是文件夹的索引
     * @return
     */
    bool getIsDir(){
        return isDirectory;
    }
    /**
     * 获得属性
     * @return
     */
    string getAttribute(){
        return this->attribute;
    }
    string getFileName(){
        return this->fileName;
    }
    int getLenOfFile(){
        return this->lengthOfFile;
    }
    string getWholeName(){
        return this->fileName+"."+this->extensionName;
    }
};

class File;

//实际存放文件夹数据的Node
class DirNode{
    string name="";//文件夹名字
    vector<File*> fileList;//文件链表s
    vector<DirNode*> subDirs;//子文件列表
    DirNode* father=NULL;//父文件夹的指针
    DirectoryEntry* info=NULL;//有关文件夹的数据，其实就是entry
public:
    DirNode(string n){
        this->name=n;
    }
    void addSubDir(DirNode* c){
        this->subDirs.push_back(c);
    }
    void addFile(File* c){
        this->fileList.push_back(c);
    }
    vector<File*> getFiles(){
        return this->fileList;
    }
    vector<DirNode*> getSubDirs(){
        return this->subDirs;
    }
    string getDirName(){
        return this->name;
    }
    int getNumOfSubDir(){
        return this->subDirs.size();
    }
    int getNumOfFiles(){
        return this->fileList.size();
    }

};
//文件（纯数据的那种）怎么组织的： 起始是一个file* file*里面有指向一串fileNode*链表的指针
//file*--->fileNode*--->fileNode*……
class File{
    string attribute="";
    int lengthOfFile=0;
    string fileName="";//不带扩展名
    string wholeName="";//带扩展名
    FileNode* head=NULL;//起始的文件节点（这其实是一个链表）
    DirNode* father=NULL;//文件所在的上级目录
    int firstClusterNum=0;
public:
    File(string n,int l){
        this->fileName=n;
        this->lengthOfFile=l;
    }
    void addHead(FileNode* h){
        this->head=h;
    }
    void setFileLen(int n){
        this->lengthOfFile=n;
    }
    void printFile();
    void setFather(DirNode* f){
        this->father=f;
    }
    string getFileName(){
        return this->fileName;
    }
    int getFileLen(){
        return this->lengthOfFile;
    }
    void setAttribute(string a){
        this->attribute=a;
    }
    string getAttribute(){
        return this->attribute;
    }
    void setFirstClusterNum(int s){
        this->firstClusterNum=s;
    }
    void setWholeName(string name){
        this->wholeName=name;
    }
    string getWholeName(){
        return this->wholeName;
    }
};
//存放实际数据的节点(放的是文件的一个扇区部分，不是完整的文件类，一个完整的文件应该由该类指针串成的链表表示)
class FileNode{
    bool isLast= false;//true->是该文件的结尾节点
    FileNode* next=NULL;
    char data[512];//每个节点可存512字节的数据
    DirectoryEntry* father=NULL;//上级目录是什么
    int attribute=0;//文件的属性从哪儿看？？？？？？？？？？？？？？？？(只有directory有属性，它作用于整个文件夹中)
public:
    FileNode(){

    }
    FileNode(int startIndex){
        for(int i=startIndex;i<startIndex+512;i++){
            data[i-startIndex]=fileContent[i];
        }
    }
    void addNext(FileNode* n){
        this->next=n;
    }
    void setIsLast(){
        isLast= true;
    }
    FileNode* getNext(){
        return this->next;
    }
    char* getFileContent(){
        return this->data;
    }

};
void File::printFile() {
    int count= this->lengthOfFile;
    FileNode* index= this->head;
    while (index!=NULL&&count>0){
        char* content=index->getFileContent();
        //cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<endl;
        for(int i=0;i<512&&count>0;i++){
            //cout<<"count="<<count<<"  ";
            cout<<content[i];
            count--;
        }

        index=index->getNext();
    }
    cout<<endl;
}
//bootLoader的class
class BPB{
public:
    int BPB_BytsPerSec=0;//每扇区字节数
    int BPB_SecPerClus=0;//每cluster的扇区数
    int BPB_RsvdSecCnt=0;//保留的扇区数
    int BPB_NumFATs=0;//FAT个数（一般为2）
    int BPB_RootEntCnt=0;//最大根目录文件数
    int BPB_TotSec=0;//扇区总数
    int BPB_FATSz=0;//每个FAT所占扇区数
    int BPB_HiddSec=0;//隐藏扇区数
    void initBPB();
};
void BPB:: initBPB(){
    this->BPB_BytsPerSec= transformBinaryToDecimal(11,12);
    this->BPB_SecPerClus= transformBinaryToDecimal(13,13);
    this->BPB_RsvdSecCnt= transformBinaryToDecimal(14,15);
    this->BPB_NumFATs= transformBinaryToDecimal(16,16);
    this->BPB_RootEntCnt= transformBinaryToDecimal(17,18);
    this->BPB_TotSec= transformBinaryToDecimal(19,20);
    this->BPB_FATSz= transformBinaryToDecimal(22,23);
    this->BPB_HiddSec= transformBinaryToDecimal(28,31);
}
/**
 * 注意，一下所有数字都是以字节为单位的（不以字节为单位的要注意换算）
 * @return
 */
int beginOfDataArea=33*512;//数据区是从第几个字节开始的
int beginOfDirectoryArea=19*512;//前面有19个cluster，每个有512字节
int numOfEntryDirectoryAreaHas=0;//根目录项区所含cluster数 数据区虽然确定在33section处开始，但是root directory的实际项数不确定
int lengthOfFile=0;//fileContent的实际长度
BPB bpb;//全局，因为只要一个
DirNode* root;//根目录 名字记为“”
/**
 * 计算各区域的起始位置（以字节为单位） 感觉有点问题
 */
void initBeginAddress(){
    //beginOfDirectoryArea=(1+bpb.BPB_NumFATs*bpb.BPB_FATSz) *bpb.BPB_SecPerClus*512;//cluster数*每cluster的扇区数*512/sec
    numOfEntryDirectoryAreaHas=(bpb.BPB_RootEntCnt * 32 + bpb.BPB_BytsPerSec - 1)/ bpb.BPB_BytsPerSec; //用于对齐，因为可能有文件名过长的情况
    //beginOfDataArea=beginOfDirectoryArea+numOfEntryDirectoryAreaHas*bpb.BPB_SecPerClus*512;
}




/**
 * 根据簇号计算该section的实际地址（第几字节）
 * @param clusterNum
 * @return
 */
int calBeginAddress(int clusterNum){
    return beginOfDataArea+(clusterNum-2)*512; //簇号是从2开始的
}

/**
 * 用于debug
 */
void printFileContent(){
    for(int i=0;i<lengthOfFile;i++){
        char temp=fileContent[i];
        string str="";
        for(int j=0;j<8;j++){
            if((temp&1)==0) str="0"+str;
            else str="1"+str;

            temp=temp>>1;
        }
        cout<<str<<endl;
    }
}

/**
 * 二进制转十进制
 * @param startIndex fileContent中该二进制串的起始位置（以字节为单位）
 * @param endIndex
 * @return
 */
int transformBinaryToDecimal(int startIndex,int endIndex){
    int result=0;
    int ret=1;
    for(int i= endIndex;i>=startIndex;i--){
        char temp=fileContent[i];
        for(int j=0;j<8;j++){
            if((temp&1)==1) result+=ret;
            ret*=2;
            temp=temp>>1;
        }
    }
    return result;
}



/**
 * 给出该Node的二进制字符串，判断是否是文件夹还是文件(no problem)
 * @param clusterBinaryStr
 * @return
 */
bool isDir(string clusterBinaryStr){
    string Dir_Attr=clusterBinaryStr.substr(11,8);
    return Dir_Attr.compare("00010000")==0;
}

/**
 * 给出下面4byte的起始字节在fileContent中的index，返回接下来32位(4byte)的二进制字符串（no problem）
 * @param startIndex 在fileContent中的index
 * @return
 */
string transformCharArrToBinaryStr(int startIndex){
    string result="";
    for(int i=startIndex;i<startIndex+4;i++){
        char temp=fileContent[i];
        string tempStr="";
        for(int j=0;j<8;j++){
            if((temp&1)==0){
                tempStr="0"+tempStr;
            }else tempStr="1"+tempStr;
            temp=temp>>1;
        }
        result+=tempStr;
    }
    return result;
}



/**
 * 给出下面512byte的起始字节在fileContent中的index，返回接下来512byte的二进制字符串（no problem）
 * @param startIndex 在fileContent中的index
 * @return
 */
string transformToSecBinary(int startIndex){
    string result="";
    for(int i=startIndex;i<startIndex+512;i++){
        char temp=fileContent[i];
        string tempStr="";
        for(int j=0;j<8;j++){
            if((temp&1)==0){
                tempStr="0"+tempStr;
            }else tempStr="1"+tempStr;
            temp=temp>>1;
        }
        result+=tempStr;
    }
    return result;
}



int FATNum[3072];//FAT1的项

string transformCharToBinaryStrIn1Byte(char temp){
    string result="";
    for(int j=0;j<8;j++){
        if((temp&1)==0){
            result="0"+result;
        }else result="1"+result;
        temp=temp>>1;
    }
    return result;
}



//debug
void printFAT(){
    for(int i=0;i<50;i++){
        cout<<"["<<i<<"]"<<"="<<FATNum[i]<<" ";
    }
    cout<<endl;
}


/**
 * 把每个cluster存的簇号放在FATNum数组中
 * 如：FATNum[2]=3 代表cluster2的下一个是cluster3
 */
void solveFAT(){
    //先把FAT的二进制提出来
    string FATStr="";
    for(int i=512;i<10*512;i++){
        FATStr+= transformCharToBinaryStrIn1Byte(fileContent[i]);
    }
    //cout<<FATStr;
    //cout<<"len="<<FATStr.length()<<endl;
    int index=0;
    for(int i=0;i<9*512*8;i+=24){
        string tempItem=FATStr.substr(i,24);
        string first=tempItem.substr(0,8);
        string second=tempItem.substr(8,4);
        first=tempItem.substr(12,4)+first;
        second=tempItem.substr(16,8)+second;
        FATNum[index++]= transformBinaryToInt(first);
        FATNum[index++]= transformBinaryToInt(second);
    }
    printFAT();
}




/**
 * 遍历文件链表（若发现子目录，则转过去给traverseDirectory    要改！！！！！！！！！！！！！！！！！！！！！！！
 * @param secNum 起始cluster(sec)号
 * @param root  父文件夹          在调用之前就要把文件的起始node放在root里面
 * @param last 该文件的上一块（链表里）
 */
void traverseFileNode(File* root,FileNode* last,int secNum){
    //cout<<"L"<<endl;
    //先找到下一个对应的FAT表项
    int nextSecNum=FATNum[secNum];
    //先判断是否是坏簇
    if(nextSecNum==4087){//0xFF7
        cout<<"Bad cluster!"<<endl;
        return ; //******************************  程序崩溃点1
    }else if(nextSecNum==0||(nextSecNum>=4080&&nextSecNum<=4086)){ //reserved FAT 0xFF0-0xFF6 unused 0x000
        cout<<"FAT error！"<<endl;  //这里只做debug用，实际中应该不能执行到这句话
        return;
    }

    int startByteIndex= calBeginAddress(secNum);//该簇对应的section的起始字节index
    string content= transformToSecBinary(startByteIndex);
    //不存在遍历着遍历着出现子文件夹的data这种情况 每个扇区是什么内容在entry的时候即决定了
    FileNode* node=new FileNode(startByteIndex);
    //cout<<"set"<<endl;
    last->addNext(node);

    if(nextSecNum>=4088&&nextSecNum<=4095){ //0xFF8-0xFFF
        //cout<<"last!"<<endl;
        node->setIsLast();
    }else{
        traverseFileNode(root,node,nextSecNum);
    }

}
//注意，有的32是byte，并不是32位，注意区分！！！！！！！！！！！ 一个entry是32byte
/**
 * 处理32byte的entry，但这个entry可能是file的信息，也可能是dir的信息，注意区分【开始的入口，给出一个entry，准备处理】
 * @param father 父目录
 * @param entries 32*8的倍数 一整个目录的字符串 若是root directory 则为512byte 一个二进制串
 */
void traverseDirectoryNode(DirNode* father,string entries){
        for(int i=0;i<entries.length()/(32*8);i++){
            string entry=entries.substr(i*32*8,32*8);
            if(entry=="0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000") break;
            //将一个目录项转为DirectoryEntry
            DirectoryEntry node(entry);
            //取出该部分对应的数据区二进制串
            int firstClusterNum=node.getFATNum();
            int firstClusterIndex= calBeginAddress(firstClusterNum); //由cluster获得首个扇区的byte index(即在fileContent中的下标)
            string firstClusterStr= transformToSecBinary(firstClusterIndex); //取出了512byte的数据
            cout<<"firstClusterNum="<<firstClusterNum<<endl;

            //现在判断取出来的是文件还是子目录
            string tempAttr=node.getAttribute();
            //cout<<"attribute="<<tempAttr<<endl;
            if(tempAttr=="00010000"){
                cout<<"SubDir!"<<endl;
                //新的entries，下面要遍历这16个entry，就是递归
                DirNode* subDir=new DirNode(node.getFileName());
                cout<<"name="<<node.getFileName()<<" len="<<node.getFileName().length()<<endl;
                father->addSubDir(subDir);
                if(node.getFileName()=="."||node.getFileName()==".."){
                    cout<<"same!"<<endl;
                    continue; //如果是上级文件夹或本文件夹，则不再遍历
                }
                traverseDirectoryNode(subDir,firstClusterStr);

            }else if(tempAttr=="00001111") {


                cout<<"of!"<<endl;
                //这里有可能要处理长文件名字的问题************************************** to be continued


            }else {
                //由文件首地址按照FAT一路找下去，知道获取完整的文件内容
                //首先要new一个file，来接纳所有的文件节点
                cout<<"file!"<<endl;
                File* childFile=new File(node.getFileName(),node.getLenOfFile());
                childFile->setAttribute(tempAttr);
                childFile->setFileLen(node.getLenOfFile());
                childFile->setWholeName(node.getWholeName());
                FileNode* temp=new FileNode(); //只是暂时充当上一个节点，之后要后移一个
                cout<<"fileName="<<childFile->getWholeName()<<" clusterNum="<<firstClusterNum<<endl;
                traverseFileNode(childFile,temp,firstClusterNum);//遍历文件,文件以childFile为开头的链表
                temp=temp->getNext();
                childFile->setFather(father);
                childFile->addHead(temp);
                father->addFile(childFile);

            }
        }




}


/**
 * 用于判断ls语句的参数-l是否合法,如果合法，返回路径名字；若不合法，则返回“error”
 * @param s
 * @param isContainLs 用于传给外面这个指令里面是否含有“ls”
 * @return
 */
string LSjudgeLegal(string s,bool& isContainLs){
    isContainLs= false;
    string c=s.substr(2); //去除初始的“ls”
    stringstream commands(c);
    string temp="";
    int pathNum=0;//用于记录路径出现了几次
    string path="";
    while (getline(commands,temp,' ')){ //不断用空格分割字符换，并放入temp中
        if(temp==" "||temp=="") continue;
        //cout<<"["<<temp<<"] ";
        //去除temp前后的空格

        //如果一个串中包含“-l”，且只能在-l之后添加多个l， l-ll这样的形式认为是错误的 但-l-l是可以的
        //先判断这部分切片里面是否含有“-l”
        int isContain=temp.find("-l");
        //cout<<isContain<<endl;
        if(isContain==-1){
            //未找到，应该是路径
            if(pathNum>0) return "error";//多于一次出现路径
            pathNum++;
            path=temp;
        }else{
            isContainLs= true;
            if(isContain>0){
                //说明temp中的-l出现在中间，且其前面均不是“-l 错误
                return "error";
            }else{
                temp=temp.substr(2);//把第一个-l截出来
                while (temp.find("-l")==0){
                    temp=temp.substr(2);
                }//一直有就一直截
                //cout<<temp.find("-l")<<endl;
                if(temp.find("-l")==-1){
                    //说明没有-l了
                    for(int j=0;j<temp.length();j++){
                        if(temp[j]!='l') return "error";
                    }
                }else{
                    return "error";
                }
            }
        }

    }
    return path;//若返回的是“”，则代表是没有提供地址的指令
}


/**
 * 处理cat中的地址
 * @param s
 * @return
 */
string catJudgeLegal(string s){
    s=s.substr(3);//去除前导cat
    stringstream commands(s);
    string temp="";
    string path="";
    while (getline(commands,temp,' ')){
        if(temp==" "||temp=="") continue;
        if(path!="") return "error";//说明出现了两个地址
        path=temp;
    }
    return path;
}

/**
 * 根据提示的绝对地址或文件名字找文件（注意:暂时先设定必须给出完整地址）    待debug！！！！！！！！！
 * @param absoluteLocation
 * @return
 */
File* findFileByLoc(string absoluteLocation){
    //先把路径找到
    stringstream locs(absoluteLocation);
    string currentLayer="";
    DirNode* index=root;//当前指针指向根目录
    vector<string> layers; //把路径按顺序放进去
    while (getline(locs,currentLayer,'/')){
        if(currentLayer==" "||currentLayer=="") continue;
        layers.push_back(currentLayer);
    }
    for(int i=0;i<layers.size();i++){
        //cout<<"currentLayer="<<layers[i]<<endl;
        if(i<layers.size()-1){
            //说明不是最后一步，是文件夹
            vector<DirNode*> subDirs=index->getSubDirs();
            bool ret= false;
            for(int j=0;j<subDirs.size();j++){
                if(subDirs[j]->getDirName()==layers[i]){
                    index=subDirs[j];
                    //cout<<"find dir"<<index->getDirName()<<endl;
                    ret= true;
                    break;
                }

            }
            if(!ret){
                //没有找到对应路径
                cout<<"Can't find path!"<<endl;//程序崩溃点！！！！！！！！！！！！！！！！！！！！！
                return NULL;
            }

        }else{
            //找当前目录中的文件
            vector<File*> files=index->getFiles();
            bool ret= false;
            for(int j=0;j< files.size();j++){
                //cout<<"this name="<<files[j]->getWholeName()<<endl;
                if(files[j]->getWholeName()==layers[i]){
                   // cout<<"find file"<<files[j]->getWholeName()<<endl;
                    return files[j];//找到了！
                }
            }
        }

    }
    return NULL;//未找到

}


DirNode* findDirByLoc(string absoluteLocation){
    if(absoluteLocation=="") return root;
    //先把路径找到
    stringstream locs(absoluteLocation);
    string currentLayer="";
    DirNode* index=root;//当前指针指向根目录
    vector<string> layers; //把路径按顺序放进去
    while (getline(locs,currentLayer,'/')){
        if(currentLayer==" "||currentLayer=="") continue;
        layers.push_back(currentLayer);
    }
    DirNode* answer=NULL;
    bool flag= false;
    for(int i=0;i<layers.size();i++){
        vector<DirNode*> subDirs=index->getSubDirs();
        if(i<layers.size()-1){
            bool ret= false;
            for(int j=0;j<subDirs.size();j++){
                if(subDirs[j]->getDirName()==layers[i]){
                    index=subDirs[j];
                    //cout<<"find index"<<index->getDirName()<<endl;
                    ret= true;
                    break;
                }

            }
            if(!ret){
                //没有找到对应路径
                cout<<"Can't find path!"<<endl;//程序崩溃点！！！！！！！！！！！！！！！！！！！！！
                return NULL;
            }
        }else{
            //最后一步
            for(int j=0;j<subDirs.size();j++){
                if(subDirs[j]->getDirName()==layers[i]){
                    answer=subDirs[j];
                    //cout<<"find answer"<<answer->getDirName();
                    break;
                }

            }
        }


    }
    if(answer==NULL){
        cout<<"can't find dir!"<<endl;//程序崩溃点！！！！！！！！！！！！！！！！！！！！
    }
    return answer;//未找到

}


/**
 * 给出文件节点，遍历输出该节点所有下属的文件和文件夹，不带数量
 * @param node
 * @return
 */
string searchAllPathInDir(DirNode* node,string fatherPath){
    string outputStr="";
    //先把当前路径数出来
    string path=fatherPath;
    path+=node->getDirName()+"/";
    outputStr+=path+":\n";
    vector<File*> f=node->getFiles();
    vector<DirNode*> subDirs=node->getSubDirs();
    for(int i=0;i<f.size();i++){
        outputStr+=f[i]->getWholeName()+"  ";
    }
    string traverseStr="";
    for(int i=0;i<subDirs.size();i++){
        outputStr+=subDirs[i]->getDirName()+"  ";
        if(subDirs[i]->getDirName()=="."||subDirs[i]->getDirName()=="..") continue;
        traverseStr+=searchAllPathInDir(subDirs[i],path);

    }
    outputStr+="\n"+traverseStr;
    return outputStr;
}

/**
 * 给出文件节点，遍历输出该节点所有下属的文件和文件夹，带数量
 * @param node
 * @return
 */
string searchAllPathInDirWithNum(DirNode* node,string fatherPath){
    string outputStr="";
    //先把当前路径数出来
    string path=fatherPath;
    path+=node->getDirName()+"/";
    outputStr+=path;
    vector<File*> f=node->getFiles();
    vector<DirNode*> subDirs=node->getSubDirs();
    int countValidFileNum=0,countValidSubDirNum=0;
    string temp="";
    for(int i=0;i<f.size();i++){
        temp+=f[i]->getWholeName()+" "+ to_string(f[i]->getFileLen())+"\n";
        countValidFileNum++;
    }
    string traverseStr="";
    for(int i=0;i<subDirs.size();i++){
        if(subDirs[i]->getDirName()=="."||subDirs[i]->getDirName()=="..") {
            temp+=subDirs[i]->getDirName()+"\n";
            continue;
        }
        temp+=subDirs[i]->getDirName()+" "+ to_string(subDirs[i]->getNumOfSubDir()-2)+" "+ to_string(subDirs[i]->getNumOfFiles())+"\n"; //子文件目录一定含有“.”和“..”，所以可直接减2

        countValidSubDirNum++;
        traverseStr+=searchAllPathInDirWithNum(subDirs[i],path);

    }
    outputStr+=" "+ to_string(countValidSubDirNum)+" "+ to_string(countValidFileNum)+":\n";
    outputStr+=temp;
    outputStr+="\n"+traverseStr;
    return outputStr;
}

int main() {
   Tool tool;
   //tool.generateFile();
// step1:将img文件以二级制的形式读入数组 fileContent中     debug时用另一套
   /* FILE * imgFile= fopen("D:/sample.ima","rb"); //*************  这里要改！！！！！！ 文件名是以“.img”结尾
    int findErrorHelper= fseek(imgFile,0,SEEK_END);
    lengthOfFile= ftell(imgFile);
    //std::cout<<lengthOfFile<<std::endl;
    rewind(imgFile);//使指针重回文件开头
    fread(fileContent,lengthOfFile,1,imgFile);//这里的ersan参数可能是反的！
    fclose(imgFile);*/

    ifstream myfile("D:\\m\\new.txt");
    string temp;
    if (!myfile.is_open())
    {
        cout << "fail!" << endl;
    }
    int index=0;
    while(getline(myfile,temp))
    {
        for(int i=0;i<temp.length();i+=2){
            char f1=temp[i];
            int num1=0;
            char f2=temp[i+1];
            int num2=0;
            string current="";
            if(f1>='0'&&f1<='9'){
                num1=f1-'0';
            }else num1=f1-'a'+10;

            if(f2>='0'&&f2<='9'){
                num2=f2-'0';
            }else num2=f2-'a'+10;

            num1=num1<<4;
            fileContent[index++]=num1+num2;


        }

    }
    myfile.close();

    //debug
//    for(int i=0;i<index;i++){
//        string current= transformCharToBinaryStrIn1Byte(fileContent[i]);
//        cout<<current<<" ";
//    }
//Step1:把对应区域的数据都初始化
    bpb.initBPB();

    //计算各区的起始位置
    initBeginAddress();
    //处理FAT
    solveFAT();
//step2:遍历目录区和数据区
//先把根目录的19-32sec的字节拿出来转换成二进制串
    string rootDirectory="";
    for(int i=19;i<=32;i++){
        int startIndex=i*512;
        rootDirectory+= transformToSecBinary(startIndex);
    }
    cout<<"len="<<rootDirectory.length()<<endl;
   // cout<<rootDirectory<<endl;
    root=new DirNode(""); //根目录的指针
    traverseDirectoryNode(root,rootDirectory);
    vector<File*> files=root->getFiles();
    //cout<<"num="<<files.size()<<endl;
    for(int i=0;i< files.size();i++){
        cout<<files[i]->getFileName()<<" len="<<files[i]->getFileLen()<<" attri="<<files[i]->getAttribute()<<endl;
        //files[i]->printFile();
    }

    vector<DirNode*> sunDirs=root->getSubDirs();
    for(int i=0;i<sunDirs.size();i++){
        cout<<"name="<<sunDirs[i]->getDirName()<<endl;
        vector<File*> sF=sunDirs[i]->getFiles();
        for(int j=0;j<sF.size();j++){
            cout<<"["<<sF[j]->getFileName()<<" len="<<sF[j]->getFileLen()<<" attri="<<sF[j]->getAttribute()<<endl;
            //sF[j]->printFile();
        }
        vector<DirNode*> dirs=sunDirs[i]->getSubDirs();
        for(int j=0;j<dirs.size();j++){
            vector<File*> f=dirs[j]->getFiles();
            for(int k=0;k<f.size();k++){
                cout<<"["<<f[k]->getFileName()<<" len="<<f[k]->getFileLen()<<" attri="<<f[k]->getAttribute()<<endl;
                //f[k]->printFile();
            }
        }
    }


    string command;
//shep3:不断输入
    while(true){
        getline(cin,command);
        if(command=="exit") break;
        else if(command.substr(0,3)=="cat"){
            //cat 后面接的必须是文件名（可包括地址）但不能是文件夹
            string respond= catJudgeLegal(command);
            cout<<"respond="<<respond<<endl;
            if(respond=="error"){
                cout<<"Unrecognized command!"<<endl; //***************************************************************程序崩溃点2！！！！！！！！！！
                continue;
            }
            File* answer= findFileByLoc(respond);
            if(answer==NULL){
                cout<<"No such file!"<<endl;
                continue;//程序崩溃点！！！！！！！！！！！！！！！！！！
            }
            answer->printFile();
        }else if(command.substr(0,2)=="ls"){
            DirNode* answer;
            string respond;
            bool isContain=false;
            if(command=="ls"){
                //特判一下没给位置参数的情况
                answer=root;
                respond="";
            }else{
                //这里还要判断是否有参数“-l” 以及参数位置是否合适

                respond= LSjudgeLegal(command,isContain);
                if(respond=="error"){
                    cout<<"Unrecognized command!"<<endl; //***************************************************************程序崩溃点2！！！！！！！！！！
                    continue;
                }
                answer= findDirByLoc(respond);
            }
            if(answer==NULL){
                cout<<"No such file!"<<endl; //程序崩溃点！！！！！！！！！！！！！
                continue;
            }
            //去除respond中最后一个"/"和后面的内容
            int endIndex= respond.rfind("/");
            if(endIndex!=-1){
                respond=respond.substr(0, endIndex+1);
            }
            if(isContain){
                //cout<<"contain!"<<endl;
                string output= searchAllPathInDirWithNum(answer,respond);
                cout<<output<<endl;
            }else{
                string output= searchAllPathInDir(answer,respond);
                cout<<output<<endl;
            }

        }else{
            cout<<"Unrecognized command!"<<endl; //***************************************************************程序崩溃点2！！！！！！！！！！！

        }
    }

}

/**
 * 总结：
 *  1.目录里的首cluster号似乎是反着存的，即第一字节是cluster号的最低字节，第二字节才是高字节(小端存储)
 *  2.数据是小端存储，但是字符串可以看到是正着存的
 *  3.看到属性是0f的entry，一般就忽略吧（结尾有fffffff0000fffffff），暂时不知道是干嘛的
 *  4.只有子目录有 .  ..子文件
 *  5.fat里面出现了0ff，00f的情况，但指代的不是section，这是干嘛的
 *  6.root directory中出现了表示文件夹的
 *  7.更改过的文件所在文件夹会出现一些看不见的文件，但是属性仍是0x20 名字是“錙UTPU~1”  to be continued 怎么解决？？？？？？
 */

