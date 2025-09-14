#include <rtime.h>
#include <cmath>
#include <algorithm>
#include <vector>
#include <future>
#include <thread>
#include "EUtoMv.h"

// External conversion function (provided elsewhere)
extern "C" {
	int mv2eu(AIScanBlock* scan_entry, int pt_offset, short db, float* voltmv, float* value, float count);
}

// Constructor
CEUtoMv::CEUtoMv()
	: m_vWp(), m_vNp(), m_nDB(1), m_fCount(-99999.0f) {
	m_vWp.clear();
	m_vNp.clear();
}

// Destructor
CEUtoMv::~CEUtoMv() {
	m_vWp.clear();
	m_vNp.clear();
}

// Main conversion method
float CEUtoMv::FindPoint(AIScanBlock* scan_entry, int pt_offset, short db, float count, float fVal, float* fOut) {
	float fMv, s;
	float h = scan_entry[pt_offset].v_range_h;
	float l = scan_entry[pt_offset].v_range_l;
	float fRange;

	// Step 1: Adjust invalid range
	if (scan_entry[pt_offset].v_range_u == 0) {  // Millivolts
		if (l == 0.0f && h == 0.0f) {
			l = -10000.0f;
			h = 10000.0f;
		} else {
			l = std::max(l, -10000.0f);
			h = std::min(h, 10000.0f);
		}
	} else {  // Volts
		if (l == 0.0f && h == 0.0f) {
			l = -10.0f;
			h = 10.0f;
		} else {
			l = std::max(l, -10.0f);
			h = std::min(h, 10.0f);
		}
	}
	fRange = std::fabs(h - l);

	// Step 2: Wide dataset (full range)
	m_vWp.clear();
	m_vNp.clear();
	m_nDB = db;
	m_fCount = count;
	GetDataSet(scan_entry, pt_offset, l, h, true);

	// Step 3: Interpolate rough MV
	fMv = GetInterpolatedPoint(fVal, m_vWp);

	// Step 4: Choose narrow range size
	if (fRange <= 5.0f) s = 0.1f;
	else if (fRange <= 10.0f) s = 0.2f;
	else if (fRange <= 1000.0f) s = 0.5f;
	else s = 2.0f;

	// Step 5: Narrow dataset (around estimated MV)
	GetDataSet(scan_entry, pt_offset, fMv - s, fMv + s, false);

	// Step 6: Final interpolation
	s = GetInterpolatedPoint(fVal, m_vNp);

	// Optional: validate with mv2eu and store in fOut
	if (fOut != nullptr) {
		float voltmv[3] = { s, 0.0f, s };
		mv2eu(scan_entry, pt_offset, m_nDB, voltmv, fOut, m_fCount);
	}
	return s;
}

// Add interpolation point
void CEUtoMv::AddPoint(const vec3& v, bool IsWidePoint) {
	if (IsWidePoint) m_vWp.push_back(v);
	else m_vNp.push_back(v);
}

// Multithreaded GetDataSet
int CEUtoMv::GetDataSet(AIScanBlock* scan_entry, int pt_offset, float fBottom, float fTop, bool IsWide) {
	const int NUM_POINTS = 100;
	const int NUM_THREADS = std::thread::hardware_concurrency();
	const int CHUNK_SIZE = NUM_POINTS / NUM_THREADS;

	float inc = std::fabs((fTop - fBottom)) / float(NUM_POINTS);
	std::vector<std::future<std::vector<vec3>>> futures;

	for (int t = 0; t < NUM_THREADS; ++t) {
		futures.push_back(std::async(std::launch::async, [=]() {
			std::vector<vec3> localPoints;
			float localBottom = fBottom + t * CHUNK_SIZE * inc;
			for (int i = 0; i < CHUNK_SIZE; ++i) {
				int globalIndex = t * CHUNK_SIZE + i;
				if (globalIndex > NUM_POINTS) break;

				float fVal = localBottom + i * inc;
				float voltmv[3] = { fVal, 0.0f, fVal };
				float value = 0.0f;

				mv2eu(scan_entry, pt_offset, m_nDB, voltmv, &value, m_fCount);
				localPoints.emplace_back(value, fVal, float(globalIndex));
			}
			return localPoints;
		}));
	}

	for (auto& f : futures) {
		auto points = f.get();
		for (const auto& pt : points) {
			AddPoint(pt, IsWide);
		}
	}
	return 0;
}

// Linear interpolation based on sorted vec3 list
float CEUtoMv::GetInterpolatedPoint(float fVal, std::vector<vec3> vp) {
#define BOUNDS(pp) { if (pp < 0) pp = 0; else if (pp >= (int)vp.size() - 1) pp = (int)vp.size() - 2; }

	int p = 0;
	std::vector<vec3> vps = vp;
	std::sort(vps.begin(), vps.end());

	for (int x = 0; x < (int)vps.size(); ++x) {
		if (fVal < vps[x].x) break;
		p = int(vps[x].z);
	}

	int p1 = p;
	BOUNDS(p1);
	int p2 = p1 + 1;

	if (p != 0 && vp[p1].x > vp[p2].x) {
		p2 = p - 1;
	}

	return linear(fVal, vp[p1], vp[p2]);
}

// Linear y = mx + b
float CEUtoMv::linear(float x, const vec3& p1, const vec3& p2) {
	double m = (p2.x - p1.x == 0.0f) ? 0.0 : (p2.y - p1.y) / (p2.x - p1.x);
	double b = p1.y - (m * p1.x);
	return float((m * x) + b);
}
