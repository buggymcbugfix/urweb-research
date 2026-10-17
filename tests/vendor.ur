fun main () : transaction page =
    s <- source "";
    return <xml><body>
      <button value="Who" onclick={fn _ => set s (VendorFfi.vendor ())}/>
      <p><dyn signal={v <- signal s; return <xml>{[v]}</xml>}/></p>
    </body></xml>
