fun draggableList title items =
    itemSources <- List.mapM source items;
    draggingItem <- source None;
    return <xml>
      <h2>Great {[title]}</h2>
      <ul>
        {List.mapX (fn itemSource => <xml>
          <li onmousedown={fn _ => set draggingItem (Some itemSource)}
              onmouseup={fn _ => set draggingItem None}
              onmouseover={fn ev => di <- get draggingItem;
                              case di of
                                  None => return ()
                                | Some di =>
                                  if not ev.Buttons.Left then
                                      (* Let go somewhere else. *)
                                      set draggingItem None
                                  else
                                      original <- get di;
                                      movedOver <- get itemSource;
                                      set di movedOver;
                                      set itemSource original;
                                      set draggingItem (Some itemSource)}>
              <dyn signal={Monad.mp cdata (signal itemSource)}/>
         </li></xml>) itemSources}
      </ul>
    </xml>

fun main () =
    bears <- draggableList "Bears" ("Pooh" :: "Paddington" :: "Rupert" :: "Edward" :: []);
    beers <- draggableList "Beers" ("Budvar" :: "Delirium Tremens" :: "Deuchars" :: []);
    boars <- draggableList "Boars" ("Sus scrofa scrofa"
                                        :: "Sus scrofa ussuricus"
                                        :: "Sus scrofa cristatus"
                                        :: "Sus scrofa taiwanus" :: []);
    return <xml>
      <head>
        <link rel="stylesheet" type="text/css" href="../../dragList.css"/>
      </head>
      <body>
        {bears}
        {beers}
        {boars}
      </body>
    </xml>
