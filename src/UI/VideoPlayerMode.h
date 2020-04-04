#pragma once

#include <memory>

class QcMultiMediaPlayer;
class VideoPlayerMode
{
public:
	VideoPlayerMode();
	~VideoPlayerMode();

protected:
	std::unique_ptr<QcMultiMediaPlayer> m_player;
};