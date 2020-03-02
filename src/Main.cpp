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
	dbg("获得Token 状态码", response.status_code);

	json::Document document;
	document.Parse(response.text.c_str());

	token = document["data"][0]["token"].Get<const char *>();
	dbg("获得Token", token);

	return token;
}

bool CheckJson()
{
	if (_access("list.json", 0) != 0)
	{
		dbg("未创建list.json");
		FILE *fp;
		errno_t err = fopen_s(&fp, "list.json", "w");

		if (err != 0)
		{
			dbg("创建list.json失败");
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
	dbg("上传", name);

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
		dbg("上传 失败", response.status_code);
		return;
	}
	dbg("上传 状态码", response.status_code);

	json::Document upload;
	upload.Parse(response.text.c_str());

	std::string key = upload["key"].Get<const char *>(),
				hash = upload["hash"].Get<const char *>();

	dbg("上传 哈希", hash);
	dbg("上传 键值", key);
	std::string url = "https://static.codemao.cn/" + key;
	dbg("下载支链", url);

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
			dbg("包括项 不保存");
			save = false;
			break;
		}
	}

	if (save)
	{
		document.PushBack(jObject, allocator);
		dbg("不包括项 保存");
	}

	json::StringBuffer buffer;
	buffer.Clear();
	json::Writer<json::StringBuffer> writer(buffer);

	document.Accept(writer);

	FILE *fp;
	errno_t err = fopen_s(&fp, "list.json", "w");
	if (err != 0)
	{
		dbg("上传 写list.json失败");
		return;
	}
	fwrite(buffer.GetString(), strlen(buffer.GetString()), 1, fp);
	fclose(fp);

	dbg("上传 写list.json成功");

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
			dbg("存在本地文件名", name);
			break;
		}
	}

	dbg("下载 Url", url);

	bool result = DownloadFiles(url.c_str(), name.c_str());
	if (result)
		dbg("下载 成功", result);
	else
		dbg("下载 失败", result);

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
		dbg("文件名称", name);
		dbg("键值", key);

		printf("\n");
	}

	return;
}

int main(int argc, char *argv[])
{
	argparse::ArgumentParser program("CodemaoDrive");

	{
		program.add_argument("method")
			.help("选择上传/下载方法")
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
			.help("要上传/下载 的 文件名/文件键值")
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