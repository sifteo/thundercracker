/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * message banners that show up to display score or game info
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _BANNER_H
#define _BANNER_H

#include <sifteo.h>

using namespace Sifteo;

class Banner
{
public:
	static const unsigned int BANNER_WIDTH = 16;
    static const unsigned int CENTER_PT = 8;
	static const int BANNER_ROWS = 4;
	static const int MAX_BANNER_LENGTH = 16;
	static const float SCORE_FADE_DELAY = 2.0f;

	Banner();

    void Draw( BG1Helper &bg1helper );
	void Update(float t, Cube &cube);

	void SetMessage( const char *pMsg, float duration = SCORE_FADE_DELAY );
	bool IsActive() const;

private:
	char m_Msg[MAX_BANNER_LENGTH];
	float m_fEndTime;
    //how many tiles of the banner to show
    unsigned int m_tiles;
};

#endif
