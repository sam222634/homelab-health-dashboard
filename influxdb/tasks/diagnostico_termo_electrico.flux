option task = {name: "diagnostico_termo_electrico", every: 1m}

clima = from(bucket: "server_room")
    |> range(start: -1m)
    |> filter(fn: (r) => r._measurement == "climate")
    |> aggregateWindow(every: 1m, fn: mean, createEmpty: false)
    |> pivot(rowKey: ["_time", "location"], columnKey: ["_field"], valueColumn: "_value")

potencia = from(bucket: "server_room")
    |> range(start: -1m)
    |> filter(fn: (r) => r._measurement == "power")
    |> aggregateWindow(every: 1m, fn: mean, createEmpty: false)
    |> pivot(rowKey: ["_time", "location"], columnKey: ["_field"], valueColumn: "_value")

join(tables: {c: clima, p: potencia}, on: ["_time", "location"])
    |> map(fn: (r) => ({
        _time: r._time,
        _measurement: "diagnostics",
        location: r.location,
        _field: "status_code",
        _value: if r.temp > 8.0 and r.current < 0.08 then "FALLO_ELECTRICO_O_TERMOSTATO"
                else if r.temp > 8.0 and r.hum > 75.0 and r.current >= 0.08 then "PUERTA_ABIERTA_O_FILTRACION"
                else if r.current >= 0.08 and exists r.pf and r.pf < 0.55 then "BAJO_FACTOR_POTENCIA_MOTOR"
                else "NORMAL"
    }))
    |> to(bucket: "server_room")
