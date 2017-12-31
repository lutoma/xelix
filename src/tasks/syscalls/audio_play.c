/* audio_play.c: Hacky temporary syscall to play audio
 * Copyright © 2016 Lukas Martini
 *
 * This file is part of Xelix.
 *
 * Xelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#include <tasks/syscall.h>
#include <hw/ac97.h>
#include <lib/log.h>

SYSCALL_HANDLER(audio_play)
{
	#ifdef ENABLE_AC97
	SYSCALL_SAFE_RESOLVE_PARAM(0);
	ac97_play(&ac97_cards[0], (char*)syscall.params[0]);
	#endif
	SYSCALL_RETURN(0);
}
