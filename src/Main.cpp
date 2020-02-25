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

enum class Method {
	DOWNLOAD = 1,
	UPLOAD = 2,
	OTHER = 3
};

bool FileExistsStatus(const char* path)
{
	DWORD dwAttribute = GetFileAttributes(path);
	if (dwAttribute == 0XFFFFFFFF) return false;
	else return true;
}

bool DownloadFiles(const char* url, const char* downloadPath)
{
	if (URLDownloadToFile(NULL, url, downloadPath, 0, 0) == S_OK && FileExistsStatus(downloadPath)) return true;
	else return false;
}

std::string GetToken() {
	std::string token;

	auto response = cpr::Get(
		cpr::Url{ TOKEN_URL },
		cpr::UserAgent{ USER_AGENT },
		cpr::VerifySsl{ false },
		cpr::Parameters{
			{"from", "static"}
		}
	);
	dbg("获得Token 状态码", response.status_code);

	json::Document document;
	document.Parse(response.text.c_str());

	token = document["data"][0]["token"].Get<const char*>();
	dbg("获得Token", token);

	return token;
}

void Upload(const std::string& name) {
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

	dbg("上传 状态码", response.status_code);
	
	json::Document document;
	document.Parse(response.text.c_str());

	dbg("上传 哈希", document["hash"].Get<const char*>());
	dbg("上传 键值", document["key"].Get<const char*>());

	return;
}

void Download(const std::string& key) {
	std::string url = "https://static.codemao.cn/" + key;
	std::string name = key;

	dbg("下载 Url", url);

	bool result = DownloadFiles(url.c_str(), name.c_str());
	if (result)
		dbg("下载 成功", result);
	else
		dbg("下载 失败", result);

	return;
}

int main(int argc, char* argv[]) {
	argparse::ArgumentParser program("CodemaoMaterial");

	{
		program.add_argument("method")
			.help("选择上传/下载方法")
			.required()
			.action([](const std::string& value) {
			if (value == "upload")
				return Method::UPLOAD;
			else if (value == "download")
				return Method::DOWNLOAD;
			else
				return Method::OTHER;
			});

		program.add_argument("-d", "--data")
			.help("要上传/下载 的 文件名/文件键值")
			.required()
			.action([](const std::string& value) { return value; });
	}

	try {
		program.parse_args(argc, argv);
	}
	catch (const std::runtime_error & err) {
		std::cout << err.what() << std::endl;
		std::cout << program;
		exit(0);
	}

	Method method = program.get<Method>("method");
	std::string data = program.get<std::string>("--data");

	dbg(method);

	switch (method)
	{
	case Method::UPLOAD: {
		Upload(data);
		break;
	}
	case Method::DOWNLOAD: {
		Download(data);
		break;
	}
	case Method::OTHER: {
		std::cout << program;
		break;
	}
	}

	return 0;
}