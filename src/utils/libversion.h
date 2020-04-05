#pragma once

namespace libstring
{
	template<class T>
	int compareVersion(const T& version1, const T& version2)
	{
		int n1 = version1.size(), n2 = version2.size();
		int i = 0, j = 0, d1 = 0, d2 = 0;
		while (i < n1 || j < n2) {
			while (i < n1 && version1[i] != '.') {
				d1 = d1 * 10 + version1[i++] - '0';
			}
			while (j < n2 && version2[j] != '.') {
				d2 = d2 * 10 + version2[j++] - '0';
			}
			if (d1 > d2) return 1;
			else if (d1 < d2) return -1;
			d1 = d2 = 0;
			++i; ++j;
		}
		return 0;
	}
}
