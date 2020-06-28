#include "SourceCoordinateHelper.h"
#include <algorithm>

static void fillAll(const source_rect* srcRect, const source_rect* dstRect, source_rect* outSrcRect, source_rect* outDstRect)
{
	*outSrcRect = *srcRect;
	*outDstRect = *dstRect;
}
static void noScale(const source_rect* srcRect, const source_rect* dstRect, source_rect* outSrcRect, source_rect* outDstRect)
{
	int iMinW = std::min(dstRect->width, srcRect->width);
	int iMinH = std::min(dstRect->height, srcRect->height);
	outSrcRect->width = iMinW;
	outSrcRect->height = iMinH;
	outSrcRect->x = srcRect->x + (srcRect->width - outSrcRect->width) / 2;
	outSrcRect->y = srcRect->y + (srcRect->height - outSrcRect->height) / 2;

	outDstRect->width = iMinW;
	outDstRect->height = iMinH;
	outDstRect->x = dstRect->x + (dstRect->width - outDstRect->width) / 2;
	outDstRect->y = dstRect->y + (dstRect->height - outDstRect->height) / 2;
}
//cut source to dst radio.
static void radioCut(const source_rect* srcRect, const source_rect* dstRect, source_rect* outSrcRect, source_rect* outDstRect)
{
	*outDstRect = *dstRect;
	double dstRatio = dstRect->width / (double)dstRect->height;
	double srcRatio = srcRect->width / (double)srcRect->height;
	if ((dstRatio - srcRatio) > 0)
	{
		outSrcRect->width = srcRect->width;
		outSrcRect->height = (unsigned)(outSrcRect->width / dstRatio + 0.5);
	}
	else
	{
		outSrcRect->height = srcRect->height;
		outSrcRect->width = (unsigned)(outSrcRect->height * dstRatio + 0.5);
	}
	outSrcRect->x = srcRect->x + (srcRect->width - outSrcRect->width) / 2;
	outSrcRect->y = srcRect->y + (srcRect->height - outSrcRect->height) / 2;
}
//fill black border
static void radioFill(const source_rect* srcRect, const source_rect* dstRect, source_rect* outSrcRect, source_rect* outDstRect)
{
	*outSrcRect = *srcRect;
	double dstRatio = dstRect->width / (double)dstRect->height;
	double srcRatio = srcRect->width / (double)srcRect->height;
	if ((dstRatio - srcRatio) < 0)
	{
		outDstRect->width = dstRect->width;
		outDstRect->height = (unsigned)(outDstRect->width / srcRatio + 0.5);
	}
	else
	{
		outDstRect->height = dstRect->height;
		outDstRect->width = (unsigned)(outDstRect->height * srcRatio + 0.5);
	}
	outDstRect->x = dstRect->x + (dstRect->width - outDstRect->width) / 2;
	outDstRect->y = dstRect->y + (dstRect->height - outDstRect->height) / 2;
}
void CalCenterMargins(const source_rect* srcRect, const source_rect* dstRect, int iScaleFalg, source_rect* outSrcRect, source_rect* outDstRect)
{
	switch (iScaleFalg)
	{
	case eFillAll:
	{
		fillAll(srcRect, dstRect, outSrcRect, outDstRect);
		break;
	}
	case eFillNoScale:
	{
		noScale(srcRect, dstRect, outSrcRect, outDstRect);
		break;
	}
	case eFillRadioCut:
	{
		radioCut(srcRect, dstRect, outSrcRect, outDstRect);
		break;
	}
	case eFillRadioFill:
	{
		radioFill(srcRect, dstRect, outSrcRect, outDstRect);
		break;
	}
	}
}
