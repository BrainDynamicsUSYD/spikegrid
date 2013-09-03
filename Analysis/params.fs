module Params
//grid dimensions
let size=100
let mods = 
    let bseq = -3::-2::-1::0::1::2::3::[]
    bseq |> List.collect (fun x -> bseq |> List.map (fun y -> x,y))
let minsize=3
type Class = 
    |Firing
    |Grouped of int
    |NotFiring
open System.Collections.Generic
let inline pointadd (a,b) (c,d) = a+c,b+d

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
let makeCoord (x,y) = 
    let rx = if x < 0 then x+size
             else if x < size then x
             else x - size
    let ry = if y < 0 then y+size
             else if y < size then y
             else y - size
    rx,ry
let inline makefloat (a,b) = float a, float b
let inline scale (a,b) v = v*a,v*b

let makeCoordf (x,y) = 
    let rx = if x < 0.0 then x+(size |> float)
             else if x < (size |> float) then x
             else x - (size |> float)
    let ry = if y < 0.0 then y+(size |> float)
             else if y < (size |> float) then y
             else y - (size |> float)
    rx,ry


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
let Center points = 
    let factor = 1.0 / (float (points |> Array.length))
    let pt1 = makefloat points.[0]
    let average = ref ((makefloat points.[0])) 
    points |> Array.iter (fun elem -> average := pointadd !average (scale (newminus (makefloat elem) pt1) factor))
    !average |> makeCoordf
