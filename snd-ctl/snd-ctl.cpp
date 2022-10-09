// Copyright (c) 2022 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <limits.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/ioctl.h>

#include <unistd.h>

#include <sound/asound.h>

static const char *const type_labels[] = {
    [SNDRV_CTL_ELEM_TYPE_NONE]          = "none",
    [SNDRV_CTL_ELEM_TYPE_BOOLEAN]       = "boolean",
    [SNDRV_CTL_ELEM_TYPE_INTEGER]       = "integer",
    [SNDRV_CTL_ELEM_TYPE_ENUMERATED]    = "enumerated",
    [SNDRV_CTL_ELEM_TYPE_BYTES]         = "bytes",
    [SNDRV_CTL_ELEM_TYPE_IEC958]        = "iec60958",
    [SNDRV_CTL_ELEM_TYPE_INTEGER64]     = "integer64",
};

static const char *const iface_labels[] = {
    [SNDRV_CTL_ELEM_IFACE_CARD]         = "card",
    [SNDRV_CTL_ELEM_IFACE_HWDEP]        = "hwdep",
    [SNDRV_CTL_ELEM_IFACE_MIXER]        = "mixer",
    [SNDRV_CTL_ELEM_IFACE_PCM]          = "pcm",
    [SNDRV_CTL_ELEM_IFACE_RAWMIDI]      = "rawmidi",
    [SNDRV_CTL_ELEM_IFACE_TIMER]        = "timer",
    [SNDRV_CTL_ELEM_IFACE_SEQUENCER]    = "sequencer",
};


static int allocate_elem_ids(int fd, struct snd_ctl_elem_list *list)
{
    struct snd_ctl_elem_id *ids;

    /* Get the number of elements in this control device. */
    if (ioctl(fd, SNDRV_CTL_IOCTL_ELEM_LIST, list) < 0)
        return -errno;

    /* No elements found. */
    if (list->count == 0)
        return 0;

    /* Allocate spaces for these elements. */
    ids = (snd_ctl_elem_id*)calloc(list->count, sizeof(struct snd_ctl_elem_id));
    if (ids == NULL)
        return -ENOMEM;

    list->offset = 0;
    while (list->offset < list->count) {
        /*
         * ALSA middleware has limitation of one operation.
         * 1000 is enought less than the limitation.
         */
        if (list->count - list->offset > 1000)
            list->space = 1000;
        else
            list->space = list->count - list->offset;
        list->pids = ids + list->offset;

        /* Get the IDs of elements in this control device. */
        if (ioctl(fd, SNDRV_CTL_IOCTL_ELEM_LIST, list) < 0) {
            free(ids);
            list->pids = NULL;
            return errno;
        }

        list->offset += list->space;
    }

    list->pids = ids;
    list->space = list->count;

    return 0;
}

static void deallocate_elem_ids(struct snd_ctl_elem_list *list)
{
    if (list->pids != NULL)
        free(list->pids);
}

static int dump_card_info(int fd)
{
    struct snd_ctl_card_info info = {0};

    if (ioctl(fd, SNDRV_CTL_IOCTL_CARD_INFO, &info) < 0) {
        printf("ioctl(2): %s\n", strerror(errno));
        return -errno;
    }

    printf("CARD_NUMBER=%d\n", info.card);
    printf("CARD_ID=%s\n", info.id);
    printf("DRIVER=%s\n", info.driver);
    printf("CARD_NAME=%s\n", info.name);
    printf("LONG_NAME=%s\n", info.longname);
    printf("MIXER_NAME=%s\n", info.mixername);
    printf("COMPONENTS=%s\n", info.components);
    return 0;
}

int main(int argc, const char *const argv[])
{
    const char *path;
    int fd;
    struct snd_ctl_elem_list list = {0};
    int err;

    if (argc < 2) {
        printf("At least one argument is required for ALSA control character "
               "device.\n");
        return EXIT_FAILURE;
    }
    path = argv[1];

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("open(2): %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    err = allocate_elem_ids(fd, &list);
    if (err < 0) {
        close(fd);
        return EXIT_FAILURE;
    }

    err = dump_card_info(fd);
    if (err < 0) {
        close(fd);
        return EXIT_FAILURE;
    }

    deallocate_elem_ids(&list);

    close(fd);

    return EXIT_SUCCESS;
}
