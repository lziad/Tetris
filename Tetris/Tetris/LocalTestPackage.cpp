#include "LocalTestPackage.h"

int interpretSeverLog(Json::Value & orig)
{
	Json::Value ret;
	//0 or 1
	int myTeam; 
	auto dataSize = orig["log"].size() / 2;
	cout << "Local: Please enter team id: "; cin >> myTeam;
	ret["requests"][0] = orig["log"][0]["output"]["content"][to_string(myTeam)];
	for (auto i = 1u; i <= dataSize; i++)
	{
		auto &seq = orig["log"][2 * i - 1][to_string(1 - myTeam)]["response"]["seq"];
		//last seq?
		ret["requests"][i] = seq[seq.size() - 1];
		ret["requests"][i]["block"] = orig["log"][2 * i - 1][to_string(1 - myTeam)]["response"]["block"];
	}
	for (auto i = 0u; i < dataSize; i++)
	{
		ret["responses"][i] = orig["log"][2 * i + 1][to_string(myTeam)]["response"];
	}
	Json::FastWriter writer;
	cout << writer.write(ret);

	orig = ret;
	return 0;
}
