#include "customOtelProtobuf.h"
#include "time.h"

// Protobuf encoding of a string
bool encode_string(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    const char *str = (const char *)(*arg);

    if (!pb_encode_tag_for_field(stream, field))
        return false;

    return pb_encode_string(stream, (const uint8_t *)str, strlen(str));
}

// Protobuf encoding of Key-Value pairs (Attributes in OpenTelemetry)
bool KeyValue_encode_attributes(pb_ostream_t *ostream, const pb_field_iter_t *field, void *const *arg)
{
    Attrptr myAttr = (Attrnode *)(*arg);

    if (myAttr != NULL)
    {
        KeyValue keyvalue = {};

        keyvalue.key.arg = myAttr->key;
        keyvalue.key.funcs.encode = encode_string;

        keyvalue.has_value = true;
        keyvalue.value.which_value = AnyValue_string_value_tag;
        keyvalue.value.value.string_value.arg = myAttr->value;
        keyvalue.value.value.string_value.funcs.encode = encode_string;

        // Build the submessage tag
        if (!pb_encode_tag_for_field(ostream, field))
        {
            const char *error = PB_GET_ERROR(ostream);
            return false;
        }

        // Build the submessage payload
        if (!pb_encode_submessage(ostream, KeyValue_fields, &keyvalue))
        {
            const char *error = PB_GET_ERROR(ostream);
            return false;
        }
    }

    return true;
}

// Protobuf encoding of a Sum datapoint
bool Sum_encode_data_points(pb_ostream_t *ostream, const pb_field_iter_t *field, void *const *arg)
{
    Dpointsptr myDataPoint = (Dpointsptr)(*arg);

    if (myDataPoint != NULL)
    {
        NumberDataPoint data_point = {};

        data_point.time_unix_nano = myDataPoint->time;

        // Two 64-bit number types in OpenTelemetry: Integers or Doubles
        if (myDataPoint->type == AS_INT)
        {
            data_point.which_value = NumberDataPoint_as_int_tag;
            data_point.value.as_int = myDataPoint->value.as_int;
        }
        else if (myDataPoint->type == AS_DOUBLE)
        {
            data_point.which_value = NumberDataPoint_as_double_tag;
            data_point.value.as_double = myDataPoint->value.as_double;
        }

        // Do we have attributes to assign to this datapoint?
        if (myDataPoint->attributes != NULL)
        {
            data_point.attributes.arg = myDataPoint->attributes;
            data_point.attributes.funcs.encode = KeyValue_encode_attributes;
        }

        // Any flags for this?
        data_point.flags = 0;

        // Build the submessage tag
        if (!pb_encode_tag_for_field(ostream, field))
        {
            const char *error = PB_GET_ERROR(ostream);
            return false;
        }

        // Build the submessage payload
        if (!pb_encode_submessage(ostream, NumberDataPoint_fields, &data_point))
        {
            const char *error = PB_GET_ERROR(ostream);
            return false;
        }
    }

    return true;
}

// Protobuf encoding of a Metric definition
bool ScopeMetrics_encode_metric(pb_ostream_t *ostream, const pb_field_iter_t *field, void *const *arg)
{
    Metricsptr myMetric = (Metricsptr)(*arg);

    if (myMetric != NULL)
    {
        Metric metric = {};

        // Metric definition
        metric.name.arg = myMetric->name;
        metric.name.funcs.encode = encode_string;

        metric.description.arg = myMetric->description;
        metric.description.funcs.encode = encode_string;

        metric.unit.arg = myMetric->unit;
        metric.unit.funcs.encode = encode_string;

        /*
         * Metric type: counter (sum, monotonic)
         * A stateful counter would be cumulative, as it increments the value
         * Here we use a delta, to indicate to increment by 1 at each pulse
         */
        // Sum
        metric.which_data = Metric_sum_tag;
        metric.data.sum.data_points.arg = myMetric->datapoints;
        metric.data.sum.data_points.funcs.encode = Sum_encode_data_points;
        // Monotonic
        metric.data.sum.is_monotonic = true;
        // Cumulative if we pass a persistent, incrementing value
        metric.data.sum.aggregation_temporality = AggregationTemporality_AGGREGATION_TEMPORALITY_CUMULATIVE;
        // Delta if we pass the difference between now and before (e.g. value = 1)
        // metric.data.sum.aggregation_temporality = AggregationTemporality_AGGREGATION_TEMPORALITY_DELTA;

        // Build the submessage tag
        if (!pb_encode_tag_for_field(ostream, field))
        {
            const char *error = PB_GET_ERROR(ostream);
            return false;
        }

        // Build the submessage payload
        if (!pb_encode_submessage(ostream, Metric_fields, &metric))
        {
            const char *error = PB_GET_ERROR(ostream);
            return false;
        }
    }

    return true;
}

// Protobuf encoding of a scope (passthrough - nothing much done here)
bool ResourceMetrics_encode_scope_metrics(pb_ostream_t *ostream, const pb_field_iter_t *field, void *const *arg)
{
    Metricsptr myMetric = (Metricsptr)(*arg);
    if (myMetric != NULL)
    {
        ScopeMetrics scope_metrics = {};

        scope_metrics.metrics.arg = myMetric;
        scope_metrics.metrics.funcs.encode = ScopeMetrics_encode_metric;

        // Build the submessage tag
        if (!pb_encode_tag_for_field(ostream, field))
        {
            const char *error = PB_GET_ERROR(ostream);
            return false;
        }

        // Build the submessage payload
        if (!pb_encode_submessage(ostream, ScopeMetrics_fields, &scope_metrics))
        {
            const char *error = PB_GET_ERROR(ostream);
            return false;
        }
    }

    return true;
}

// Protobuf encoding of the top level (resourcemetrics)
bool MetricsData_encode_resource_metrics(pb_ostream_t *ostream, const pb_field_iter_t *field, void *const *arg)
{
    Rmetricsptr myDataPtr = (Rmetricsptr)(*arg);

    if (myDataPtr != NULL)
    {
        ResourceMetrics resource_metrics = {};

        resource_metrics.has_resource = true;

        if (myDataPtr->attributes != NULL)
        {
            resource_metrics.resource.attributes.arg = myDataPtr->attributes;
            resource_metrics.resource.attributes.funcs.encode = KeyValue_encode_attributes;
        }

        if (myDataPtr->metrics != NULL)
        {
            resource_metrics.scope_metrics.arg = myDataPtr->metrics;
            resource_metrics.scope_metrics.funcs.encode = ResourceMetrics_encode_scope_metrics;
        }

        // Build the submessage tag
        if (!pb_encode_tag_for_field(ostream, field))
        {
            const char *error = PB_GET_ERROR(ostream);
        }

        // Build the submessage payload
        if (!pb_encode_submessage(ostream, ResourceMetrics_fields, &resource_metrics))
        {
            const char *error = PB_GET_ERROR(ostream);
            return false;
        }
    }
    return true;
}

/* The main() for protobuf:
 *   - data is assigned to the top-level encoding function
 *   - stream is created for the output, which points to a storage container
 *   - encoding is then performed (multiple passes, cascading thru encode methods)
 *   - data is written to output stream, along with size, status, etc
 *   - data is available in storage container
 */
size_t buildProtobuf(Rmetricsptr args, uint8_t *pbufPayload, size_t pbufMaxSize)
{
    MetricsData metricsdata = {};

    metricsdata.resource_metrics.arg = args;
    metricsdata.resource_metrics.funcs.encode = MetricsData_encode_resource_metrics;

    pb_ostream_t output = pb_ostream_from_buffer(pbufPayload, pbufMaxSize);
    int pbufStatus = pb_encode(&output, MetricsData_fields, &metricsdata);
    size_t pbufPayloadSize = output.bytes_written;

    // if there's a pbuf error
    if (!pbufStatus)
        return 0;

    return pbufPayloadSize;
}

// Get the system time (needs to be set via RTC/ NTP)
uint64_t getEpochNano(void)
{
    time_t epoch;
    time(&epoch);
    return (uint64_t) epoch * 1000000000;
}

// Data structure functions - create (malloc)
Rmetricsptr createDataPtr(void) {
    // Fake dataset
    Attrptr tempattrb = (Attrptr)malloc(sizeof(Attrnode));
    tempattrb->key = "location";
    tempattrb->value = "downstairs";

    Dpointsptr tempdp = (Dpointsptr)malloc(sizeof(Dpointnode));
    // Update these from the main program
    tempdp->time = 0;
    tempdp->value.as_int = 0;
    // Keep these static
    tempdp->type = AS_INT;
    tempdp->attributes = tempattrb;

    Metricsptr tempmetrics = (Metricsptr)malloc(sizeof(Metricnode));
    tempmetrics->name = "smartmeter";
    tempmetrics->description = "Honeywell AS302P";
    tempmetrics->unit = "1";
    tempmetrics->datapoints = tempdp;

    Attrptr tempattra = (Attrptr)malloc(sizeof(Attrnode));
    tempattra->key = "service.name";
    tempattra->value = "house-power";

    Rmetricsptr myDataPtr = (Rmetricsptr)malloc(sizeof(Rmetricsnode));
    myDataPtr->metrics = tempmetrics;
    myDataPtr->attributes = tempattra;

    return myDataPtr;
}

// Data structure functions - release (free memory)
void releaseDataPtr(Rmetricsptr myDataPtr) {
    // Assign data to each pointer type
    Attrptr tempattra = myDataPtr->attributes;
    Metricsptr tempmetrics = myDataPtr->metrics;
    Dpointsptr tempdp = tempmetrics->datapoints;
    Attrptr tempattrb = tempdp->attributes;

    // Can now free() each pointer
    free(tempattrb);
    free(tempdp);
    free(tempmetrics);
    free(tempattra);
    free(myDataPtr);
}