/*	$OpenBSD$	*/
/*
 * Copyright (c) 2008 Alexandre Ratchov <alex@caoua.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef SNDIO_H
#define SNDIO_H

#include <sys/types.h>

/*
 * default audio device and MIDI port
 */
#define SIO_DEVANY	"default"
#define MIO_PORTANY	"default"
#define SIOMIX_DEVANY	"default"

/*
 * limits
 */
#define SIOMIX_NAMEMAX		12	/* max name length */
#define SIOMIX_INTMAX		127	/* max channel number */
#define SIOMIX_HALF		64	/* also bool threshold */

/*
 * private ``handle'' structure
 */
struct sio_hdl;
struct mio_hdl;
struct siomix_hdl;

/*
 * parameters of a full-duplex stream
 */
struct sio_par {
	unsigned int bits;	/* bits per sample */
	unsigned int bps;	/* bytes per sample */
	unsigned int sig;	/* 1 = signed, 0 = unsigned */
	unsigned int le;	/* 1 = LE, 0 = BE byte order */
	unsigned int msb;	/* 1 = MSB, 0 = LSB aligned */
	unsigned int rchan;	/* number channels for recording direction */
	unsigned int pchan;	/* number channels for playback direction */
	unsigned int rate;	/* frames per second */
	unsigned int bufsz;	/* end-to-end buffer size */
#define SIO_IGNORE	0	/* pause during xrun */
#define SIO_SYNC	1	/* resync after xrun */
#define SIO_ERROR	2	/* terminate on xrun */
	unsigned int xrun;	/* what to do on overruns/underruns */
	unsigned int round;	/* optimal bufsz divisor */
	unsigned int appbufsz;	/* minimum buffer size */
	int __pad[3];		/* for future use */
	unsigned int __magic;	/* for internal/debug purposes only */
};

/*
 * capabilities of a stream
 */
struct sio_cap {
#define SIO_NENC	8
#define SIO_NCHAN	8
#define SIO_NRATE	16
#define SIO_NCONF	4
	struct sio_enc {			/* allowed sample encodings */
		unsigned int bits;
		unsigned int bps;
		unsigned int sig;
		unsigned int le;
		unsigned int msb;
	} enc[SIO_NENC];
	unsigned int rchan[SIO_NCHAN];	/* allowed values for rchan */
	unsigned int pchan[SIO_NCHAN];	/* allowed values for pchan */
	unsigned int rate[SIO_NRATE];	/* allowed rates */
	int __pad[7];			/* for future use */
	unsigned int nconf;		/* number of elements in confs[] */
	struct sio_conf {
		unsigned int enc;	/* mask of enc[] indexes */
		unsigned int rchan;	/* mask of chan[] indexes (rec) */
		unsigned int pchan;	/* mask of chan[] indexes (play) */
		unsigned int rate;	/* mask of rate[] indexes */
	} confs[SIO_NCONF];
};

#define SIO_XSTRINGS { "ignore", "sync", "error" }

/*
 * subset of channels of a stream
 */
struct siomix_chan {
	char str[SIOMIX_NAMEMAX];	/* stream name */
	int unit;			/* optional stream number */
};

/*
 * description of a control (index, value) pair
 */
struct siomix_desc {
	unsigned int addr;		/* control address */
#define SIOMIX_NUM		2	/* integer in the 0..127 range */
#define SIOMIX_SW		3	/* on/off switch (0 or 1) */
#define SIOMIX_VEC		4	/* number, element of vector */
#define SIOMIX_LIST		5	/* switch, element of a list */
	unsigned int type;		/* one of above */
	char group[SIOMIX_NAMEMAX];	/* or class name */
	char func[SIOMIX_NAMEMAX];	/* function name */
	struct siomix_chan chan0;	/* affected channels */
	struct siomix_chan chan1;	/* dito for SIOMIX_{VEC,LIST} */
};

/*
 * mode bitmap
 */
#define SIO_PLAY	1
#define SIO_REC		2
#define MIO_OUT		4
#define MIO_IN		8
#define SIOMIX_READ	0x100
#define SIOMIX_WRITE	0x200

/*
 * default bytes per sample for the given bits per sample
 */
#define SIO_BPS(bits) (((bits) <= 8) ? 1 : (((bits) <= 16) ? 2 : 4))

/*
 * default value of "sio_par->le" flag
 */
#if BYTE_ORDER == LITTLE_ENDIAN
#define SIO_LE_NATIVE 1
#else
#define SIO_LE_NATIVE 0
#endif

/*
 * maximum value of volume, eg. for sio_setvol()
 */
#define SIO_MAXVOL 127

#ifdef __cplusplus
extern "C" {
#endif

struct pollfd;

void sio_initpar(struct sio_par *);
struct sio_hdl *sio_open(const char *, unsigned int, int);
void sio_close(struct sio_hdl *);
int sio_setpar(struct sio_hdl *, struct sio_par *);
int sio_getpar(struct sio_hdl *, struct sio_par *);
int sio_getcap(struct sio_hdl *, struct sio_cap *);
void sio_onmove(struct sio_hdl *, void (*)(void *, int), void *);
size_t sio_write(struct sio_hdl *, const void *, size_t);
size_t sio_read(struct sio_hdl *, void *, size_t);
int sio_start(struct sio_hdl *);
int sio_stop(struct sio_hdl *);
int sio_nfds(struct sio_hdl *);
int sio_pollfd(struct sio_hdl *, struct pollfd *, int);
int sio_revents(struct sio_hdl *, struct pollfd *);
int sio_eof(struct sio_hdl *);
int sio_setvol(struct sio_hdl *, unsigned int);
int sio_onvol(struct sio_hdl *, void (*)(void *, unsigned int), void *);

struct mio_hdl *mio_open(const char *, unsigned int, int);
void mio_close(struct mio_hdl *);
size_t mio_write(struct mio_hdl *, const void *, size_t);
size_t mio_read(struct mio_hdl *, void *, size_t);
int mio_nfds(struct mio_hdl *);
int mio_pollfd(struct mio_hdl *, struct pollfd *, int);
int mio_revents(struct mio_hdl *, struct pollfd *);
int mio_eof(struct mio_hdl *);

struct siomix_hdl *siomix_open(const char *, unsigned int, int);
void siomix_close(struct siomix_hdl *);
int siomix_ondesc(struct siomix_hdl *,
    void (*)(void *, struct siomix_desc *, int), void *);
int siomix_onctl(struct siomix_hdl *,
    void (*)(void *, unsigned int, unsigned int), void *);
int siomix_setctl(struct siomix_hdl *, unsigned int, unsigned int);
int siomix_nfds(struct siomix_hdl *);
int siomix_pollfd(struct siomix_hdl *, struct pollfd *, int);
int siomix_revents(struct siomix_hdl *, struct pollfd *);
int siomix_eof(struct siomix_hdl *);

#ifdef __cplusplus
}
#endif

#endif /* !defined(SNDIO_H) */
