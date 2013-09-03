module OrderParam
open System.Collections.Generic
open System.Linq
open Params
open System.IO
type rollingWindow (count,v:(int*int)[] []) = 
    let arr = Array.create count v
    let mutable i = 0
    member x.first = arr.[i] //For later on I need to be able to extract a certain wave and then filter it out - this is probably the best way
    member x.add z = 
        i <- (i+1) % count
        arr.[i] <- z
    member x.iteri f = 
        Array.iteri (fun j v -> f ((count - j + i) % count) v) arr

let smoothsize = 5
let wwindow = new rollingWindow(smoothsize,[||])

let classify (d:(int*int)[]) =
    let Fpoints = HashSet<int*int>()
    let mutable group = 0
    let newarr = Array2D.create size size NotFiring
    let mutable ind = 0
    d |> Array.iter (fun (x,y) -> newarr.[x,y] <- Firing;Fpoints.Add(x,y) |> ignore)
    
    let mutable ret = [] //some empty arrays so that seq.take works
    while Fpoints.Count <> 0 do
        let p1,p2 = (Fpoints :> IEnumerable<_>).First()
        newarr.[p1,p2]<-Grouped(group)
        let gp = neighbours (p1,p2) newarr group
        gp.Add(p1,p2)
        let ggp=gp.ToArray()
        ggp |> Array.iter (fun t -> Fpoints.Remove t |> ignore)
        group <- group + 1
        ret <- ggp :: ret
    ret <- ret |> List.filter (fun t -> t |> Array.length > 3)
    group,ret 

let angles (wave:((int*int) []),center:float*float) =
    wave
    |> Array.map( fun (c,d) ->
        let (a,b) = newminus center (float c, float d)
        let theta = atan2 b a
        (cos(theta), sin(theta)))

let getfcoords (input:System.IO.StreamReader) =
    match input.ReadLine() with
    |null -> [||]
    |t->t.Split(';')
        |> Array.filter (fun t -> t <> "")
        |> Array.choose (fun t -> 
                match t.Split(',') with
                |[|a;b|] -> Some((int a),(int b))
                |_ ->     None  
                )
let DoOrderParam outstream pstream = 
    let points = getfcoords pstream
    let first = classify points |> snd |> List.toArray
    if first <> [||] then
        let dummywave = [||] //the important thing here is that the point list is empty
        let centers = first |> Array.map Center
        let map = first |> Array.mapi (fun i (w) -> centers.[i], Array.init smoothsize (function |0 -> w | _ -> dummywave)) |> Map.ofArray //this is very hackish - but the non w entries are overwritten in the next loop
        wwindow.iteri (fun i elem ->
            if i <> 0 then
                elem 
                |> Array.iter (fun w ->
                    let newcenter = centers |> Array.minBy (fun c -> newminus c (Center w) |> fun (a,b) -> a*a+b*b)
                    let dist =  newminus newcenter (Center w) |> fun (a,b) -> a*a+b*b
                    map.[newcenter].[i] <- if dist < (smoothsize*smoothsize |> float) then w else dummywave))
        map
        |> Map.toList
        |> List.map ( fun (c,w) ->w |> Array.collect (fun wa -> angles (wa, c)))
        |> List.filter (fun v -> Array.length v > 20)
        |> fun v ->
            if v <> [] then
                v
                |> List.averageBy (fun f -> 
                    f
                    |> Array.fold (fun (a,b) (c,d) -> (a+c,b+d)) (0.0, 0.0)
                    |> fun (a,b) -> sqrt(a*a+b*b)/(float (f|> Array.length))) |> fprintfn outstream "%f" 
            else fprintfn outstream "%f" 0.0


[<EntryPoint>]
let main args =
    match args with
    |[|f|] ->
        use pointReader = new StreamReader(f);
        use outwrite = System.IO.File.CreateText(f.Replace("input","order"))
        outwrite.AutoFlush <- true
        while not (pointReader.EndOfStream) do
            DoOrderParam outwrite pointReader
        0
