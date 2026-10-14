val step : transaction int
val attempts : transaction int
val arm : transaction unit
val inTxn : transaction bool
val watch : string -> transaction unit
val watchAfter : string -> transaction unit
