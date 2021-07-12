/*
 * Xelix audio play and grab interface
 * Copyright (c) 2021 Lukas Martini
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "config.h"

#include <string.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>

#include "libavutil/log.h"

#include "libavcodec/avcodec.h"
#include "avdevice.h"

#include "xelixsnd.h"

int ff_xelixsnd_audio_open(AVFormatContext *s1, int is_output,
                      const char *audio_device)
{
    XelixSNDAudioData *s = s1->priv_data;
    int audio_fd;

    if (is_output) {
        audio_fd = avpriv_open(audio_device, O_WRONLY);
    } else {
        av_log(s1, AV_LOG_ERROR, "xelixsnd: Audio input is not supported\n");
        return AVERROR(EIO);
    }

    if (audio_fd < 0) {
        av_log(s1, AV_LOG_ERROR, "%s: %s\n", audio_device, av_err2str(AVERROR(errno)));
        return AVERROR(EIO);
    }

    s->frame_size = XELIXSND_AUDIO_BLOCK_SIZE;
    s->codec_id = AV_CODEC_ID_PCM_S16LE;
    s->sample_rate = 44100;
    s->fd = audio_fd;

    return 0;
}

int ff_xelixsnd_audio_close(XelixSNDAudioData *s)
{
    close(s->fd);
    return 0;
}
