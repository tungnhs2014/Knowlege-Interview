#include <stdbool.h>
#include <stdio.h>

typedef void (*SampleCallback)(void *context, int sample);

typedef struct {
    SampleCallback function;
    void *context;
} SampleListener;

typedef struct {
    const char *name;
    unsigned int count;
    int latest;
} SampleStats;

static bool listener_notify(
    const SampleListener *listener,
    int sample)
{
    if (listener == NULL || listener->function == NULL) {
        return false;
    }

    listener->function(listener->context, sample);
    return true;
}

static void listener_clear(SampleListener *listener)
{
    if (listener != NULL) {
        listener->function = NULL;
        listener->context = NULL;
    }
}

static void record_sample(void *context, int sample)
{
    SampleStats *stats = context;

    if (stats != NULL) {
        ++stats->count;
        stats->latest = sample;
    }
}

int main(void)
{
    SampleStats stats = {"sensor-a", 0U, 0};
    SampleListener listener = {record_sample, &stats};

    if (!listener_notify(&listener, 21)
        || !listener_notify(&listener, 24)) {
        return 1;
    }

    printf("%s count=%u latest=%d\n",
           stats.name,
           stats.count,
           stats.latest);

    listener_clear(&listener);
    printf("after-unregister=%s\n",
           listener_notify(&listener, 99) ? "called" : "rejected");
    return 0;
}
