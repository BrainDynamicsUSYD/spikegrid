open System.Collections.Generic
open System.IO
open Arrayops
open Params
open System.Linq


let readline (parsed:System.IO.StreamReader) =
    match parsed.ReadLine() with
    |null -> [||]
    |t->t.Split(':')
        |> Array.filter (fun t -> t <> "")
        |> Array.choose (fun t -> 
                match t.Split(',') with
                |[|a;b|] -> Some((int a),(int b))
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
