#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <intrin.h>
#include <iphlpapi.h>
#include "machineid.h"
#include <sstream>

#pragma comment(lib, "IPHLPAPI.lib")

namespace machineid {
	// we just need this for purposes of unique machine id. So any one or two mac's is
	// fine.
	unsigned short int hashMacAddress(PIP_ADAPTER_INFO info) {
		unsigned short int hash = 0;
		for (unsigned int i = 0; i < info->AddressLength; i++) {
			hash += (info->Address[i] << ((i & 1) * 8));
		}
		return hash;
	}

	void getMacHash(unsigned short int& mac1, unsigned short int& mac2) {
		IP_ADAPTER_INFO AdapterInfo[32];
		DWORD dwBufLen = sizeof(AdapterInfo);

		DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);
		if (dwStatus != ERROR_SUCCESS)
			return; // no adapters.

		PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
		mac1 = hashMacAddress(pAdapterInfo);
		if (pAdapterInfo->Next)
			mac2 = hashMacAddress(pAdapterInfo->Next);

		// sort the mac addresses. We don't want to invalidate
		// both macs if they just change order.
		if (mac1 > mac2) {
			unsigned short int tmp = mac2;
			mac2 = mac1;
			mac1 = tmp;
		}
	}

	unsigned short int getVolumeHash() {
		DWORD serialNum = 0;

		// Determine if this volume uses an NTFS file system.
		GetVolumeInformation(L"c:\\", NULL, 0, &serialNum, NULL, NULL, NULL, 0);
		unsigned short int hash = (unsigned short int)((serialNum + (serialNum >> 16)) & 0xFFFF);

		return hash;
	}

	unsigned short int getCpuHash() {
		int cpuinfo[4] = { 0, 0, 0, 0 };
		__cpuid(cpuinfo, 0);
		unsigned short int hash = 0;
		unsigned short int* ptr = (unsigned short int*)(&cpuinfo[0]);
		for (unsigned int i = 0; i < 8; i++)
			hash += ptr[i];

		return hash;
	}

	const char* getMachineName() {
		static char computerName[1024];
		DWORD size = 1024;
		GetComputerNameA(computerName, &size);
		return &(computerName[0]);
	}


	std::string generateHash(const std::string& bytes) {
		static char chars[] = "0123456789ABCDEF";
		std::stringstream stream;

		auto size = bytes.size();
		for (unsigned long i = 0; i < size; ++i) {
			unsigned char ch = ~((unsigned char)((unsigned short)bytes[i] +
				(unsigned short)bytes[(i + 1) % size] +
				(unsigned short)bytes[(i + 2) % size] +
				(unsigned short)bytes[(i + 3) % size])) * (i + 1);

			stream << chars[(ch >> 4) & 0x0F] << chars[ch & 0x0F];
		}

		return stream.str();
	}


	static std::string* cachedHash = nullptr;

	std::string machineHash() {
		static const unsigned long TargetLength = 64;

		if (cachedHash != nullptr) {
			return *cachedHash;
		}

		std::stringstream stream;

		stream << getMachineName();
		stream << getCpuHash();
		stream << getVolumeHash();

		auto string = stream.str();

		while (string.size() < TargetLength) {
			string = string + string;
		}

		if (string.size() > TargetLength) {
			string = string.substr(0, TargetLength);
		}

		return generateHash(string);
	}
}
