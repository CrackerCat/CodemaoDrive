#include <cpr/cpr.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <argparse.hpp>
#include <dbg.h>

#include <iostream>
#include <fstream>
#include <windows.h>
#include <corecrt_io.h>

#pragma comment(lib, "urlmon.lib")

#define TOKEN_URL "https://api.codemao.cn/api/v2/cdn/upload/token/1"
#define UPLOAD_URL "https://upload.qiniup.com/"

#define USER_AGENT "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/39.0.2171.71 Safari/537.36"

namespace json = rapidjson;

enum class Method
{
	DOWNLOAD = 1,
	UPLOAD = 2,
	HISTORY = 3,
	OTHER = 4
};

bool FileExistsStatus(const char *path)
{
	DWORD dwAttribute = GetFileAttributes(path);
	if (dwAttribute == 0XFFFFFFFF)
		return false;
	else
		return true;
}

bool DownloadFiles(const char *url, const char *downloadPath)
{
	if (URLDownloadToFile(NULL, url, downloadPath, 0, 0) == S_OK && FileExistsStatus(downloadPath))
		return true;
	else
		return false;
}

std::string GetPathShortName(std::string strFullName)
{
	auto string_replace = [](std::string &strBig, const std::string &strsrc, const std::string &strdst) {
		std::string::size_type pos = 0;
		std::string::size_type srclen = strsrc.size();
		std::string::size_type dstlen = strdst.size();

		while ((pos = strBig.find(strsrc, pos)) != std::string::npos)
		{
			strBig.replace(pos, srclen, strdst);
			pos += dstlen;
		}
	};

	if (strFullName.empty())
	{
		return "";
	}

	string_replace(strFullName, "/", "\\");
	std::string::size_type iPos = strFullName.find_last_of('\\') + 1;

	return strFullName.substr(iPos, strFullName.length() - iPos);
}

std::string GetToken()
{
	std::string token;

	auto response = cpr::Get(
		cpr::Url{TOKEN_URL},
		cpr::UserAgent{USER_AGENT},
		cpr::VerifySsl{false},
		cpr::Parameters{
			{"from", "static"}});
	dbg("���Token ״̬��", response.status_code);

	json::Document document;
	document.Parse(response.text.c_str());

	token = document["data"][0]["token"].Get<const char *>();
	dbg("���Token", token);

	return token;
}

bool CheckJson()
{
	if (_access("list.json", 0) != 0)
	{
		dbg("δ����list.json");
		FILE *fp;
		errno_t err = fopen_s(&fp, "list.json", "w");

		if (err != 0)
		{
			dbg("����list.jsonʧ��");
			fclose(fp);
			return false;
		}
		fprintf(fp, "[]");
		fclose(fp);
	}
	return true;
}

void Upload(const std::string &name)
{
	dbg("�ϴ�", name);

	std::string token = GetToken();
	auto response = cpr::Post(
		cpr::Url{UPLOAD_URL},
		cpr::UserAgent{USER_AGENT},
		cpr::VerifySsl{false},
		cpr::Multipart{
			{"token", token},
			{"file", cpr::File{name}}});

	if (response.status_code != 200)
	{
		dbg("�ϴ� ʧ��", response.status_code);
		return;
	}
	dbg("�ϴ� ״̬��", response.status_code);

	json::Document upload;
	upload.Parse(response.text.c_str());

	std::string key = upload["key"].Get<const char *>(),
				hash = upload["hash"].Get<const char *>();

	dbg("�ϴ� ��ϣ", hash);
	dbg("�ϴ� ��ֵ", key);
	std::string url = "https://static.codemao.cn/" + key;
	dbg("����֧��", url);

	std::ifstream fdata("list.json");
	std::istreambuf_iterator<char> begin(fdata);
	std::istreambuf_iterator<char> end;
	std::string fileData(begin, end);
	fdata.close();

	json::Document document;
	json::Document::AllocatorType &allocator = document.GetAllocator();
	document.Parse(fileData.c_str());

	json::Value jObject(json::kObjectType);

	std::string shortName = GetPathShortName(name);
	json::Value jName(json::kStringType);
	jName.SetString(shortName.c_str(), shortName.size());
	jObject.AddMember("name", jName, allocator);

	json::Value jKey(json::kStringType);
	jKey.SetString(key.c_str(), key.size());
	jObject.AddMember("key", jKey, allocator);

	bool save = true;

	for (auto &m : document.GetArray())
	{
		if (std::string(m["key"].GetString()) == key)
		{
			dbg("������ ������");
			save = false;
			break;
		}
	}

	if (save)
	{
		document.PushBack(jObject, allocator);
		dbg("�������� ����");
	}

	json::StringBuffer buffer;
	buffer.Clear();
	json::Writer<json::StringBuffer> writer(buffer);

	document.Accept(writer);

	FILE *fp;
	errno_t err = fopen_s(&fp, "list.json", "w");
	if (err != 0)
	{
		dbg("�ϴ� дlist.jsonʧ��");
		return;
	}
	fwrite(buffer.GetString(), strlen(buffer.GetString()), 1, fp);
	fclose(fp);

	dbg("�ϴ� дlist.json�ɹ�");

	return;
}

void Download(const std::string &key)
{
	std::string url = "https://static.codemao.cn/" + key;
	std::string name = key;

	std::ifstream fdata("list.json");
	std::istreambuf_iterator<char> begin(fdata);
	std::istreambuf_iterator<char> end;
	std::string fileData(begin, end);
	json::Document document;
	document.Parse(fileData.c_str());

	for (auto &m : document.GetArray())
	{
		if (std::string(m["key"].GetString()) == key)
		{
			name = m["name"].GetString();
			dbg("���ڱ����ļ���", name);
			break;
		}
	}

	dbg("���� Url", url);

	bool result = DownloadFiles(url.c_str(), name.c_str());
	if (result)
		dbg("���� �ɹ�", result);
	else
		dbg("���� ʧ��", result);

	return;
}

void History()
{
	std::ifstream fdata("list.json");
	std::istreambuf_iterator<char> begin(fdata);
	std::istreambuf_iterator<char> end;
	std::string fileData(begin, end);

	json::Document document;
	document.Parse(fileData.c_str());

	for (auto &m : document.GetArray())
	{
		std::string name = m["name"].GetString(),
					key = m["key"].GetString();
		dbg("�ļ�����", name);
		dbg("��ֵ", key);

		printf("\n");
	}

	return;
}

int main(int argc, char *argv[])
{
	argparse::ArgumentParser program("CodemaoDrive");

	{
		program.add_argument("method")
			.help("ѡ���ϴ�/���ط���")
			.required()
			.action([](const std::string &value) {
				if (value == "upload")
					return Method::UPLOAD;
				else if (value == "download")
					return Method::DOWNLOAD;
				else if (value == "history")
					return Method::HISTORY;
				else
					return Method::OTHER;
			});

		program.add_argument("-d", "--data")
			.help("Ҫ�ϴ�/���� �� �ļ���/�ļ���ֵ")
			.action([](const std::string &value) { return value; });
	}

	try
	{
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error &err)
	{
		std::cout << err.what() << std::endl;
		std::cout << program;
		exit(0);
	}

	CheckJson();

	Method method = program.get<Method>("method");
	dbg(method);

	switch (method)
	{
	case Method::UPLOAD:
	{
		std::string data = program.get<std::string>("--data");
		Upload(data);
		break;
	}
	case Method::DOWNLOAD:
	{
		std::string data = program.get<std::string>("--data");
		Download(data);
		break;
	}
	case Method::HISTORY:
	{
		History();
		break;
	}
	case Method::OTHER:
	{
		std::cout << program;
		break;
	}
	}

	return 0;
}