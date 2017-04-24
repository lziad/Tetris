#include "LocalTestPackage.h"

int interpretSeverLog(Json::Value & orig)
{
	if (orig["log"].type() == Json::nullValue)
		return 1;
	Json::Value ret;
	//0 or 1
	int myTeam;
	auto dataSize = orig["log"].size() / 2;
	cout << "Local: Please enter team id: "; cin >> myTeam;
	ret["requests"][0] = orig["log"][0]["output"]["content"][to_string(myTeam)];
	for (auto i = 1u; i <= dataSize; i++)
	{
		ret["requests"][i] = orig["log"][2 * i - 1][to_string(1 - myTeam)]["response"];
	}
	for (auto i = 0u; i < dataSize; i++)
	{
		ret["responses"][i] = orig["log"][2 * i + 1][to_string(myTeam)]["response"];
	}

	//Json::FastWriter writer;cout << writer.write(ret);

	orig = ret;
	return 0;
}


