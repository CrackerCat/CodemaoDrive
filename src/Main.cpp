#include <cpr/cpr.h>
#include <rapidjson/document.h>
#include <argparse.hpp>
#include <dbg.h>

#include <iostream>
#include <fstream>
#include <windows.h>

#pragma comment(lib, "urlmon.lib")

#define TOKEN_URL "https://api.codemao.cn/api/v2/cdn/upload/token/1"
#define UPLOAD_URL "https://upload.qiniup.com/"

#define USER_AGENT "Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/39.0.2171.71 Safari/537.36"

namespace json = rapidjson;

std::string GetToken()
{
	std::string token;

	auto response = cpr::Get(
		cpr::Url{ TOKEN_URL },
		cpr::UserAgent{ USER_AGENT },
		cpr::VerifySsl{ false },
		cpr::Parameters{
			{"from", "static"}
		}
	);
	dbg("Token状态码", response.status_code);

	json::Document document;
	document.Parse(response.text.c_str());

	token = document["data"][0]["token"].Get<const char*>();
	dbg("Token", token);

	return token;
}

void Upload(const std::string& name)
{
	dbg("上传", name);

	std::string token = GetToken();
	auto response = cpr::Post(
		cpr::Url{ UPLOAD_URL },
		cpr::UserAgent{ USER_AGENT },
		cpr::VerifySsl{ false },
		cpr::Multipart{
			{"token", token},
			{"file", cpr::File{name}}
		}
	);

	dbg("上传状态码", response.status_code);
	
	json::Document document;
	document.Parse(response.text.c_str());

	dbg("哈希", document["hash"].Get<const char*>());
	dbg("键值", document["key"].Get<const char*>());

	return;
}

void Download(const std::string& key)
{
	std::string url = "https://static.codemao.cn/" + key;
	dbg("正在下载Url", url);

	HRESULT result = URLDownloadToFile(NULL, url.c_str(), key.c_str(), 0, NULL);
	if (result == S_OK)
		dbg("下载成功", result);
	else
		dbg("下载失败", result);

	return;
}

int main(int argc, char* argv[])
{
	argparse::ArgumentParser program("CodemaoMaterial");

	{
		program.add_argument("-m", "--method")
			.help("选择上传/下载方法")
			.required()
			.action([](const std::string& value) { return value; })
			.default_value("download");

		program.add_argument("-n", "--name")
			.help("要上传的文件名")
			.action([](const std::string& value) { return value; })
			.default_value(false);

		program.add_argument("-k", "--key")
			.help("要下载的文件键值")
			.action([](const std::string& value) { return value; })
			.default_value(false);
	}

	try
	{
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error & err)
	{
		std::cout << err.what() << std::endl;
		std::cout << program;
		exit(0);
	}

	std::string key, filename;
	std::string method = program.get<std::string>("--method");
	if (method == "upload")
	{
		filename = program.get<std::string>("--name");
		Upload(filename);
	}
	else if (method == "download")
	{
		key = program.get<std::string>("--key");
		Download(key);
	}
	else
	{
		exit(0);
	}

	return 0;
}