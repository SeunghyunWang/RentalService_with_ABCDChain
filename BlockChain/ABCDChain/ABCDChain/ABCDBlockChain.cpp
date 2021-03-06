//
//  main.cpp
//  Private_Blockchain_forDB
//
//  Created by HwaSeok Shin on 2018. 6. 22..
//  Copyright © 2018년 HwaSeok Shin. All rights reserved.
//
#define ASIO_STANDALONE

#include <iostream>
#include <fstream>
#include <list>
#include <cstdlib>
#include <memory>
#include <utility>

#include "ABCDBlock.hpp"
#include "Transaction.hpp"
#include "json/json.h"
#include "asio.hpp"
//#include "secp256k1/secp256k1.h"
//#include "secp256k1/secp256k1_recovery.h"

using asio::ip::tcp;

std::list<ABCDBlock*> blockList;
Json::Value nodeAddressList;
std::string myIp = "";

char* HexStringToByteArray(std::string str)
{
    if(str.size() % 2 != 0)
        return NULL;
    
    char *a = new char[str.size() / 2 + 1];
    for(int i = 0 ; i < str.size() / 2; i++)
    {
        int front = str.at(i * 2);
        int back = str.at(i*2 + 1);
        
        if(front >= 'a')
            front = front - 'a' + 10;
        else if(front >= 'A')
            front = front - 'A' + 10;
        else
            front -= '0';
        
        if(back >= 'a')
            back = back - 'a' + 10;
        else if(back >= 'A')
            back = back - 'A' + 10;
        else
            back -= '0';
        
        
        a[i] = (front << 4) + back;
    }
    a[str.size() / 2 + 1] = 0;
    
    return a;
}

double GetAmountOfAddress(std::string address)
{
    std::list<ABCDBlock*>::iterator blockIt;
    std::list<Transaction>::iterator transIt;
	std::list<Transaction> transList;
    double amount = 0;
    
    for(blockIt = blockList.begin(); blockIt != blockList.end(); blockIt++)
    {
		transList = (*blockIt)->GetTransactionList();

		for (transIt = transList.begin(); transIt != transList.end(); transIt++)
		{
			if (transIt->GetTransactionType() == Issue &&
				transIt->GetAddress2() == address)
				amount += transIt->GetAmount();
			else if (transIt->GetTransactionType() == Payment &&
				transIt->GetAddress1() == address)
				amount -= transIt->GetAmount();
		}
    }
    
    return amount;
}
bool TransactionVerify(Transaction transaction)
{
    std::list<ABCDBlock*>::iterator blockIt;
    std::list<Transaction>::iterator transIt;
	std::list<Transaction> transList;
    
    double amount = 0;
    bool isRentaled = false;
	std::string RentalAddress;
    
    switch (transaction.GetTransactionType()) {
            
        case WalletRegistration:
            for(blockIt = blockList.begin(); blockIt != blockList.end(); blockIt++)
            {
				transList = (*blockIt)->GetTransactionList();

				for (transIt = transList.begin(); transIt != transList.end(); transIt++)
				{
                    if(transIt->GetTransactionType() == WalletRegistration &&
                       transIt->GetAddress1() == transaction.GetAddress1())
                        return false;
                }
            }
            return true;
        
        case Issue:
            for(blockIt = blockList.begin(); blockIt != blockList.end(); blockIt++)
            {
				transList = (*blockIt)->GetTransactionList();

				for (transIt = transList.begin(); transIt != transList.end(); transIt++)
				{
                    if(transIt->GetTransactionType() == WalletRegistration &&
                       transIt->GetAddress1() == transaction.GetAddress1())
                        return true;
                }
            }
            return false;
            
        case Payment:
            return GetAmountOfAddress(transaction.GetAddress1()) >= transaction.GetAmount();
            
        case Rental:
            
            for(blockIt = blockList.begin(); blockIt != blockList.end(); blockIt++)
            {
				transList = (*blockIt)->GetTransactionList();

				for (transIt = transList.begin(); transIt != transList.end(); transIt++)
				{
                    if(transIt->GetTransactionType() == Issue &&
                       transIt->GetAddress2() == transaction.GetAddress1())
                    {
                        amount += transIt->GetAmount();
                    }
                    else if(transIt->GetTransactionType() == Payment &&
                            transIt->GetAddress1() == transaction.GetAddress1())
                    {
                        amount -= transIt->GetAmount();
                    }
                    
                    if(transIt->GetTransactionType() == Rental &&
                       transIt->GetDeviceId() == transaction.GetDeviceId())
                    {
                        isRentaled = true;
                    }
                    else if(transIt->GetTransactionType() == Return &&
                            transIt->GetDeviceId() == transaction.GetDeviceId())
                    {
                        isRentaled = false;
                    }
                }
            }
            
            if(!isRentaled && amount >= transaction.GetAmount())
                return true;
            else
                return false;
            
        case Return:
            
            for(blockIt = blockList.begin(); blockIt != blockList.end(); blockIt++)
            {
				transList = (*blockIt)->GetTransactionList();

				for (transIt = transList.begin(); transIt != transList.end(); transIt++)
				{
                    if(transIt->GetTransactionType() == Issue &&
                       transIt->GetAddress2() == transaction.GetAddress1())
                    {
                        amount += transIt->GetAmount();
                    }
                    else if(transIt->GetTransactionType() == Payment &&
                            transIt->GetAddress1() == transaction.GetAddress1())
                    {
                        amount -= transIt->GetAmount();
                    }
                    
                    if(transIt->GetTransactionType() == Rental &&
                       transIt->GetDeviceId() == transaction.GetDeviceId())
                    {
						RentalAddress = transIt->GetAddress1();
                        isRentaled = true;
                    }
                    else if(transIt->GetTransactionType() == Return &&
                            transIt->GetDeviceId() == transaction.GetDeviceId())
                    {
                        isRentaled = false;
                    }
                }
            }
            
            if(isRentaled && RentalAddress == transaction.GetAddress1())// && amount >= transaction.GetAmount())
                return true;
            else
                return false;
            
            break;
    }
    
    return false;
}
bool BlockVerify()
{
	std::list<ABCDBlock*>::iterator nowIt;
	std::list<ABCDBlock*>::iterator preIt;

	for (nowIt = blockList.begin(); nowIt != blockList.end(); nowIt++)
	{
		if ((*nowIt)->GetBlockId() == 0)
		{
			preIt = nowIt;
			continue;
		}

		if ((*nowIt)->GetPreviousHash() != (*preIt)->GetBlockHash())
			return false;

		preIt = nowIt;
	}

	return true;
}

bool isRentalAvailable(std::string deviceId)
{
    std::list<ABCDBlock*>::iterator blockIt;
    std::list<Transaction>::iterator transIt;
    std::list<Transaction> transList;
    bool isRent = false;
    
    for(blockIt = blockList.begin(); blockIt != blockList.end(); blockIt++)
    {
        transList = (*blockIt)->GetTransactionList();
        
        for (transIt = transList.begin(); transIt != transList.end(); transIt++)
        {
            if (transIt->GetTransactionType() == Rental &&
                transIt->GetDeviceId() == deviceId)
                isRent = true;
            else if (transIt->GetTransactionType() == Return &&
                     transIt->GetDeviceId() == deviceId)
                isRent = false;
        }
    }
	return !isRent;
}

std::string RentedByAdress(std::string address)
{
    std::list<ABCDBlock*>::iterator blockIt;
    std::list<Transaction>::iterator transIt;
    std::list<Transaction> transList;
    std::string deviceId = "";
    
    for(blockIt = blockList.begin(); blockIt != blockList.end(); blockIt++)
    {
        transList = (*blockIt)->GetTransactionList();
        
        for (transIt = transList.begin(); transIt != transList.end(); transIt++)
        {
            if (transIt->GetTransactionType() == Rental &&
                transIt->GetAddress1() == address)
                deviceId = transIt->GetDeviceId();
            else if (transIt->GetTransactionType() == Return &&
                     transIt->GetAddress1() == address &&
                     transIt->GetDeviceId() == deviceId)
                deviceId = "";
        }
    }
    
    return deviceId != "" ? deviceId : NULL;
}

Json::Value GetBlockChainJsonValue()
{
    Json::Value root;
    int index = 0;
    std::list<ABCDBlock*>::iterator it;
    for(it = blockList.begin(); it != blockList.end(); it++)
        root[index++] = (*it)->GetJsonValue();
    
    return root;
}

void WriteBlockToFile()
{
    std::ofstream outFile;
    outFile.open("ABCDBlockNode.json");
    
    outFile << GetBlockChainJsonValue();
    //std::cout << GetBlockChainJsonValue();
    std::cout << "Complete!" << std::endl;
    
    outFile.close();
}


bool MakeBlockChainFromJsonValue(Json::Value jsonValue)
{
    try {
        for (int i = 0; i < jsonValue.size(); i++)
        {
            ABCDBlock *tmpBlock = new ABCDBlock(jsonValue[i]);
            blockList.push_back(tmpBlock);
        }
        WriteBlockToFile();
    } catch (std::exception) {
        return false;
    }
    return true;
}


bool ReadBlockFromFile()
{
    std::ifstream inFile;
    inFile.open("ABCDBlockNode.json");
    
    if (inFile.fail())
    {
        std::cout << "ABCDChain : No block node on this program's directory" << std::endl;
        return false;
    }
    
    Json::Value jsonValue;
    
    inFile >> jsonValue;
    
    inFile.close();
    
    MakeBlockChainFromJsonValue(jsonValue);
    
    return true;
}

void BroadCastLastBlock()
{
    asio::io_service clientIoService;
    for(int i = 0; i < nodeAddressList.size(); i++)
    {
        asio::ip::tcp::endpoint endPoint(asio::ip::address::from_string
                                         (nodeAddressList[i]["IP address"].asString()),
                                         nodeAddressList[i]["Port"].asUInt());
        asio::error_code connectError;
        asio::ip::tcp::socket socket(clientIoService);
        
        socket.connect(endPoint, connectError);
        if(connectError)
        {
            std::cout << "ABCDChain Block broadcast : Node ip \'" << nodeAddressList[i]["IP address"].asString() <<
            "' Connect fail." << std::endl;
            nodeAddressList.removeIndex(i, new Json::Value());
        }
        else
        {
            if(myIp == "")
                myIp = socket.local_endpoint().address().to_string();
                
            std::cout << "ABCDChain : Node ip \'" << nodeAddressList[i]["IP address"].asString() <<
            "' Connected." << std::endl;
            
            Json::Value packet;
            Json::Value header;
            Json::Value body;
            
            header["Type"] = "BlockBroadCast";
            body["Block"] = blockList.back()->GetJsonValue();
            packet["Header"] = header;
            packet["Body"] = body;
            
            std::string packetStr = packet.toStyledString();
            asio::write(socket , asio::buffer(packetStr.c_str(), packetStr.size()));
        }
        
        if(socket.is_open())
            socket.close();
    }
}

void AddTransaction(Transaction trans)
{
    ABCDBlock *newBlock = new ABCDBlock(blockList.back()->GetBlockId()+1, blockList.back()->GetBlockHash());
    blockList.push_back(newBlock);
    blockList.back()->AddTransaction(trans);
    blockList.back()->Determine();

    BroadCastLastBlock();
    WriteBlockToFile();
}

bool ReadNodeAddressFile()
{
    std::ifstream inFile;
    inFile.open("NodeAddress.json");
    if(inFile.fail())
    {
        std::cout << "ABCDChain : There is no nodes to init" << std::endl;
        return false;
    }
    nodeAddressList.clear();
    
    inFile >> nodeAddressList;
    
    inFile.close();
    
    if(nodeAddressList.empty())
    {
        std::cout << "ABCDChain : There is no nodes to init" << std::endl;
        return false;
    }
    
    return true;
}
bool WriteNodeAddressFile()
{
    std::ofstream outFile;
    outFile.open("NodeAddress.json");
    
    outFile << nodeAddressList;
    std::cout << "NodeAddressfile write success" << std::endl;
    
    outFile.close();
    
    return true;
}

void BroadCastThisNode(asio::ip::tcp::socket *socket)
{
    std::string bufferStr = "";
    Json::Value packet;
    Json::Value header;
    Json::Value body;
    
    header["Type"] = "NodeBroadCast";
    packet["Header"] = header;
    packet["Body"] = body;
    std::string packetStr = packet.toStyledString();
    asio::write(*socket, asio::buffer(packetStr.c_str(), packetStr.size()));
}
void LoadBlockChainFromOtherNode(asio::ip::tcp::socket *socket)
{
    char buff[4096] = "";
    std::string bufferStr = "";
    Json::Value packet;
    Json::Value header;
    Json::Value body;
    
    header["Type"] = "FullBlockRequest";
    packet["Header"] = header;
    packet["Body"] = body;
    std::string packetStr = packet.toStyledString();
    asio::write(*socket, asio::buffer(packetStr.c_str(), packetStr.size()));
    asio::error_code ec;
    
    while(true)
    {
        asio::read(*socket, asio::buffer(buff, 4096), ec);
        bufferStr += std::string(buff);
        if(ec)
            break;
    }
    
    Json::Value BlockChain;
    
    Json::CharReaderBuilder builder;
    Json::CharReader* reader = builder.newCharReader();
    
    std::string errstr;
    bool parseSuccess = reader->parse(bufferStr.c_str(), bufferStr.c_str() + bufferStr.size(),
                                      &BlockChain, &errstr);
    delete reader;
    
    if(!parseSuccess)
    {
        std::cout << "ABCDChain error : no json style packet arrive : " << bufferStr << std::endl;
        return;
    }
    else
        std::cout << BlockChain << std::endl;
    
    std::cout << "Success to download block on other node." << std::endl;
    
    MakeBlockChainFromJsonValue(BlockChain);
}

class session
: public std::enable_shared_from_this<session>
{
public:
    session(tcp::socket socket)
    : socket_(std::move(socket))
    {
    }
    
    void start()
    {
        do_read();
    }
    
private:
    void do_read()
    {
        auto self(shared_from_this());
        socket_.async_read_some(asio::buffer(data_, max_length),
                                [this, self](std::error_code ec, std::size_t length)
                                {
                                    if (!ec)
                                    {
                                        Json::CharReaderBuilder builder;
                                        Json::CharReader* reader = builder.newCharReader();
                                        
                                        Json::Value jsonValue;
                                        
                                        std::string errstr;
                                        bool parseSuccess = reader->parse(data_, data_ + strlen(data_),
                                                                          &jsonValue, &errstr);
                                        delete reader;
                                        
                                        if(!parseSuccess)
                                        {
                                            std::cout << "ABCDChain error : no json style packet arrive : " << data_ << std::endl;
                                            return;
                                        }
                                        else
                                            std::cout << jsonValue << std::endl;
                                        
                                        try {
                                            
                                            Json::Value header = jsonValue["Header"];
                                            Json::Value body = jsonValue["Body"];
                                            
                                            Json::Value packet;
                                            Json::Value packetHeader;
                                            Json::Value packetBody;
                                            
                                            if(header["Type"] == "TransactionVerifing")
                                            {
                                                Json::Value transaction = body["Transaction"];
                                                Transaction trans(transaction);
                                                if(TransactionVerify(trans))
                                                    asio::write(socket_, asio::buffer("True", 4));
                                                else
                                                    asio::write(socket_, asio::buffer("False", 5));
                                            }
                                            else if(header["Type"] == "BlockBroadCast")
                                            {
                                                Json::Value block = body["Block"];
                                                ABCDBlock *tBlock = new ABCDBlock(block.toStyledString());
                                                blockList.push_back(tBlock);
                                                if(BlockVerify())
                                                {
                                                    WriteBlockToFile();
                                                    std::cout << "ABCDChain : Block Confirm!" << std::endl;
                                                }
                                                else
                                                {
                                                    blockList.pop_back();
                                                    delete tBlock;
                                                }
                                            }
                                            else if(header["Type"] == "FullBlockRequest")
                                            {
                                                std::string fullBlock = GetBlockChainJsonValue().toStyledString();
                                                asio::write(socket_, asio::buffer(fullBlock.c_str(), fullBlock.size()));
                                                std::cout << "ABCDBlock : Send full Block to \'" << socket_.remote_endpoint().address().to_string() <<
                                                "\' Success" << std::endl;
                                            }
                                            else if(header["Type"] == "NodeBroadCast")
                                            {
                                                Json::Value ip;
                                                ip["IP address"] = socket_.remote_endpoint().address().to_string();
                                                ip["Port"] = socket_.remote_endpoint().port();
                                                if(nodeAddressList.toStyledString().find(ip["IP address"].asString()) == std::string::npos &&
                                                   socket_.local_endpoint().address().to_string() != ip["IP address"].asString())
                                                {
                                                    nodeAddressList[nodeAddressList.size()] = ip;
                                                    WriteNodeAddressFile();
                                                    
                                                    std::cout << ip["IP address"].asString() << " Broad Casted";
                                                }
                                                
                                                packetHeader["Type"] = "NodeRespone";
                                                packet["Header"] = packetHeader;
                                                
                                                Json::Value packetBody;
                                                packet["Body"] = packetBody;
                                                
                                                std::string jsonStr = packet.toStyledString();
                                                
                                                std::cout << jsonStr << std::endl;
                                                
                                                strcpy(data_, jsonStr.c_str());
                                                do_write(jsonStr.size());
                                            }
                                            else if(header["Type"] == "NodeRespone")
                                            {
                                                Json::Value ip;
                                                ip["IP address"] = socket_.remote_endpoint().address().to_string();
                                                ip["Port"] = socket_.remote_endpoint().port();
                                                if(nodeAddressList.toStyledString().find(ip["IP address"].asString()) == std::string::npos &&
                                                   socket_.local_endpoint().address().to_string() != ip["IP address"].asString())
                                                {
                                                    nodeAddressList[nodeAddressList.size()] = ip;
                                                    WriteNodeAddressFile();
                                                    
                                                    std::cout << ip["IP address"].asString() << " Broad Cast Responed";
                                                }
                                            }
                                            else if(header["Type"] == "WalletRegist")
                                            {
                                                Transaction trans(body["Transaction"]);
                                                packetHeader["Type"] = "WalletRegistResponse";
                                                if(TransactionVerify(trans))
                                                {
                                                    AddTransaction(trans);
                                                    Transaction creditIssue(Issue, "Superkey", trans.GetAddress1(), 1000, 0);
                                                    AddTransaction(creditIssue);
                                                    
                                                    packetBody["Response"] = 1;                                                }
                                                else
                                                    packetBody["Response"] = 0;
                                                
                                                packet["Header"] = packetHeader;
                                                packet["Body"] = packetBody;
                                                
                                                std::string str = packet.toStyledString();
                                                strncpy(data_, str.c_str(), str.size());
                                                do_write(str.size());
                                            }
                                            else if(header["Type"] == "InitialConnect")
                                            {
                                                packetHeader["Type"] = "InitialConnectResponse";
                                                packetBody["Amount"] = GetAmountOfAddress(body["UserAddress"].asString());
                                                packetBody["DeviceId"] = RentedByAdress(body["UserAddress"].asString());
                                                packetBody["UserAddress"] = body["UserAddress"].asString();
                                                packetBody["Available"] = isRentalAvailable(body["DeviceId"].asString());
                                                packetBody["DevicePrice"] = body["DevicePrice"].asInt();
                                                
                                                packet["Header"] = packetHeader;
                                                packet["Body"] = packetBody;
                                                
                                                std::string str = packet.toStyledString();
                                                strncpy(data_, str.c_str(), str.size());
                                                do_write(str.size());
                                            }
                                            else if(header["Type"] == "GetAddresssAmount")
                                            {
                                                packetHeader["Type"] = "GetAddressAmountRespone";
                                                packetBody["Amount"] = GetAmountOfAddress(body["UserAddress"].asString());
                                                packetBody["DeviceId"] = RentedByAdress(body["UserAddress"].asString());
                                                packetBody["UserAddress"] = body["UserAddress"].asString();
                                                
                                                packet["Header"] = packetHeader;
                                                packet["Body"] = packetBody;
                                                
                                                std::string str = packet.toStyledString();
                                                strncpy(data_, str.c_str(), str.size());
                                                do_write(str.size());
                                            }
                                            else if(header["Type"] == "RentalRequest")
                                            {
                                            
                                                Transaction trans(body["Transaction"]);
                                                
                                                packetHeader["Type"] = "RentalResponse";
                                                packetBody["UserAddress"] = trans.GetAddress1();
                                                packetBody["RentalDevice"] = trans.GetDeviceId();
                                                packetBody["RentalTime"] = (Json::UInt64) trans.GetTime1();
                                                packetBody["DueTime"] = (Json::UInt64)trans.GetTime2();
                                                packetBody["Amount"] = trans.GetAmount();
                                                
                                                if(TransactionVerify(trans))
                                                {
                                                    AddTransaction(trans);
                                                    packetBody["Response"] = 1;
                                                }
                                                else
                                                    packetBody["Response"] = 0;
                                                
                                                std::string str = packet.toStyledString();
                                                strncpy(data_, str.c_str(), str.size());
                                                do_write(str.size());
                                            }
                                            else if(header["Type"] == "ReturnRequest")
                                            {
                                                Transaction trans(body["Transaction"]);
                                                
                                                packetHeader["Type"] = "ReturnResponse";
                                                packetBody["UserAddress"] = trans.GetAddress1();
                                                packetBody["RentalDevice"] = trans.GetDeviceId();
                                                packetBody["DueTime"] = (Json::UInt64) trans.GetTime1();
                                                packetBody["ReturnTime"] = (Json::UInt64)trans.GetTime2();
                                                packetBody["Amount"] = trans.GetAmount();
                                                
                                                if(TransactionVerify(trans))
                                                {
                                                    AddTransaction(trans);
                                                    packetBody["Response"] = 1;
                                                }
                                                else
                                                    packetBody["Response"] = 0;
                                                
                                                std::string str = packet.toStyledString();
                                                strncpy(data_, str.c_str(), str.size());
                                                do_write(str.size());
                                            }
                                            else if(header["Type"] == "CreaditIssue")
                                            {
                                                Transaction trans(body["Transaction"]);
                                                
                                                if(TransactionVerify(trans))
                                                {
                                                    AddTransaction(trans);
                                                }
                                            }
                                        }catch(std::exception e)
                                        {
                                            std::cout << "ABCDChain error : " << e.what() << std::endl;
                                            std::cout << "Packet was wrong protocol. \n: " << jsonValue << std::endl;
                                        };
                                    }
                                });
    }
    
    void do_write(std::size_t length)
    {
        auto self(shared_from_this());
        asio::async_write(socket_, asio::buffer(data_, length),
                          [this, self](std::error_code ec, std::size_t /*length*/)
                          {
                              if (!ec)
                              {
                                  std::cout << "ABCDChain : Response Complete" << std::endl;
                                  std::cout << data_ << std::endl;
                                  //do_read();
                              }
                          });
    }
    
    tcp::socket socket_;
    enum { max_length = 4096 };
    char data_[max_length];
};

class server
{
public:
    server(asio::io_service& io_service, short port)
    : acceptor_(io_service, tcp::endpoint(tcp::v4(), port)),
    socket_(io_service)
    {
        do_accept();
    }
    
private:
    void do_accept()
    {
        acceptor_.async_accept(socket_,
                               [this](std::error_code ec)
                               {
                                   if (!ec)
                                   {
                                       std::make_shared<session>(std::move(socket_))->start();
                                   }
                                   
                                   do_accept();
                               });
    }
    
    tcp::acceptor acceptor_;
    tcp::socket socket_;
};


int main(int argc, const char * argv[]) {

    bool hasLiveNode = false;
    
    
    if(ReadNodeAddressFile())
    {
        asio::io_service clientIoService;
        for(int i = 0; i < nodeAddressList.size(); i++)
        {
            asio::ip::tcp::endpoint endPoint(asio::ip::address::from_string
                                             (nodeAddressList[i]["IP address"].asString()),
                                             nodeAddressList[i]["Port"].asInt());
            asio::error_code connectError;
            asio::ip::tcp::socket socket(clientIoService);
            socket.connect(endPoint, connectError);
            if(connectError)
            {
                std::cout << "ABCDChain : Node ip \'" << nodeAddressList[i]["IP address"].asString() <<
                "' Connect fail." << std::endl;
                nodeAddressList.removeIndex(i, new Json::Value());
            }
            else
            {
                std::cout << "ABCDChain : Node ip \'" << nodeAddressList[i]["IP address"].asString() <<
                "' Connected." << std::endl;
                hasLiveNode = true;
            }
            
            if(socket.is_open())
                socket.close();
        }
    }
    //else
    {
        
        if(!ReadBlockFromFile())
        {
            std::cout << "This is first start of this node" << std::endl <<
            "1 : Make Genesis node on this node" << std::endl << "2 : Input other node's address" << std::endl << ":";
            
            while(true)
            {
                int input;
                std::cin >> input;
                
                if(input == 1)
                {
                    ABCDBlock *GenesisBlock = new ABCDBlock(0, "");
                    blockList.push_back(GenesisBlock);
                    GenesisBlock->Determine();
                    
                    WriteBlockToFile();
                    break;
                }
                else if(input == 2)
                {
                    std::string ip;
                    uint port;
                    std::cout << "Please type other node's\nip :";
                    std::cin >> ip;
                    std::cout << "port : ";
                    std::cin >> port;
                    
                    asio::io_service clientIoService;
                    asio::ip::tcp::endpoint endPoint(asio::ip::address::from_string(ip), port);
                    asio::error_code connectError;
                    asio::ip::tcp::socket socket(clientIoService);
                    
                    socket.connect(endPoint, connectError);
                    if(connectError)
                    {
                        std::cout << "ABCDChain : Node ip \'" << ip <<
                        "' Connect fail. Please restart Node." << std::endl;
                        exit(1);
                    }
                    else
                    {
                        std::cout << "ABCDChain : Node ip \'" << ip <<"' Connected." << std::endl;
                        int index = nodeAddressList.size();
                        nodeAddressList[index]["IP address"] = ip;
                        nodeAddressList[index]["Port"] = port;
                        WriteNodeAddressFile();
                        
                        LoadBlockChainFromOtherNode(&socket);
                        BroadCastThisNode(&socket);
                    }
                    
                    if(socket.is_open())
                        socket.close();
                    
                    break;
                }
                else
                    std::cout << "Wrong number." << std::endl;
            }
        }
        else
        {
            
        }
    }
    
    asio::io_service serverIoService;
    
    server s(serverIoService, 7777);
    
    std::cout << "ABCDChain : Start node listner" << std::endl;
    
    serverIoService.run();
    
    int outs;
    std::cin>>outs;
    
    return 0;
}
