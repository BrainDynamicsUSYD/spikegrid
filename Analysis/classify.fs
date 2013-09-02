module Classify
open System.Collections.Generic
open System.IO
open Arrayops
open Params
open System.Linq
let inline makefloat (a,b) = float a, float b
let inline pointadd (a,b) (c,d) = a+c,b+d
let inline scale (a,b) v = v*a,v*b
let inline pointsub (a,b) (c,d) = a-c,b-d
let newminus a b =
    let ax,ay = pointsub a b
    let rx = 
        if ax > (float size / 2.0) then ax - (float size)  
        else if ax < (-float size / 2.0) then ax + float size
        else ax
    let ry = 
        if ay > (float size / 2.0) then ay - (float size)
        else if ay < (-float size / 2.0) then ay + float size 
        else ay
    rx,ry
let makeCoordf (x,y) = 
    let rx = if x < 0.0 then x+(size |> float)
             else if x < (size |> float) then x
             else x - (size |> float)
    let ry = if y < 0.0 then y+(size |> float)
             else if y < (size |> float) then y
             else y - (size |> float)
    rx,ry
type Class = 
    |Firing
    |Grouped of int
    |NotFiring
let makeCoord (x,y) = 
    let rx = if x < 0 then x+size
             else if x < size then x
             else x - size
    let ry = if y < 0 then y+size
             else if y < size then y
             else y - size
    rx,ry
let Center points = 
    let factor = 1.0 / (float (points |> Array.length))
    let pt1 = makefloat points.[0]
    let average = ref ((makefloat points.[0])) 
    points |> Array.iter (fun elem -> average := pointadd !average (scale (newminus (makefloat elem) pt1) factor))
    !average |> makeCoordf
let neighbours point (arr:Class[,]) group=
    let queue = Queue<_>()
    let ret = List<_>()
    queue.Enqueue(point)
    while queue.Count <> 0 do
        let pt = queue.Dequeue()
        let n = mods 
                |> List.map (pointadd pt >> makeCoord) 
                |> List.filter (fun (x,y) -> arr.[x,y] = Firing )
        n |> List.iter (fun (x,y) -> arr.[x,y] <- Grouped(group))
        n |> List.iter (ret.Add >> ignore)
        n |> List.iter (queue.Enqueue)
    ret
let readline (parsed:System.IO.StreamReader) =
    match parsed.ReadLine() with
    |null -> [||]
    |t->t.Split(';')
        |> Array.filter (fun t -> t <> "")
        |> Array.choose (fun t -> 
                match t.Split(',') with
                |[|a;b|] -> Some((int a)-1,(int b)-1)
                |_ ->     None  
                )
let classify reader writer=
    let Fpoints = HashSet<int*int>()
    let mutable group = 0
    let newarr = Array2D.create size size NotFiring
    let mutable ind = 0
    let f = readline reader
    f |> Array.iter (fun (x,y) -> newarr.[x,y]<- Firing;Fpoints.Add(x,y) |> ignore)
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
    ret <- ret |> List.filter (fun t -> t |> Array.length > minsize)
    ret |> List.iter (fun t ->
            let c1,c2 = Center t
            fprintf writer  "%4.4f,%4.4f;" c1 c2 
        )
    fprintfn writer "" 
//end of finding waves stuff
    
let main_ (f:string) =
    use reader = new StreamReader(f)
    use output = System.IO.File.CreateText(f.Replace("input","data"))
    output.AutoFlush <- true
    let mutable i = 0
    while not (reader.EndOfStream) do
        i <- i+1
        printfn "%i" i
        classify reader output
let main__ () =
    System.IO.Directory.GetFiles("input") |> Array.iter main_
[<EntryPoint>]
let main args=
    match args with 
    |[||] -> 
        main__()
    |[|a|] -> main_ a
    0     
