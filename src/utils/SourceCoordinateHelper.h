#pragma once

#if defined(__cplusplus)
extern "C" {
#endif

	enum QeFillFlag
	{
		eFillAll,
		eFillRadioCut, //cut
		eFillRadioFill, //fill black border
		eFillNoScale,
	};
	struct source_rect {
		int x;
		int y;
		int width;
		int height;
	};
	void CalCenterMargins(const source_rect* srcRect, const source_rect* dstRect, int iScaleFalg, source_rect* outSrcRect, source_rect* outDstRect);
#if defined(__cplusplus)
}
#endif
