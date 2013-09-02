module Params
//grid dimensions
let size=100
let mods = 
    let bseq = -3::-2::-1::0::1::2::3::[]
    bseq |> List.collect (fun x -> bseq |> List.map (fun y -> x,y))
let minsize=3
