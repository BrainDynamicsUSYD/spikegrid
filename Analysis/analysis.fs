//The types used for storing information
module Parser3
open Microsoft.FSharp.Collections
type LL<'t> = System.Collections.Generic.List<'t>
type HS<'t> = System.Collections.Generic.HashSet<'t>
type Location = float32*float32
type intpoint = int*int
type Wave = Location List
let inline MakeWave pos = pos::[]
let inline modWave (old:Wave) loc = loc::old
//PARAMETERS TO MODIFY
[<Literal>] //maximum value of t
let length = 180000
[<Literal>] //grid dimensions
let Size = 100
[<Literal>] //maximum value of tau to calculate.  It might be possible to set this to length-min as the code is now much faster
let maxtau = 10000
[<Literal>] //For the first few timesteps, the random initial conditions dominate.  Throw out all data for t<min
let min = 00
//maximum distance that patterns may travel in a timestep.  
//If the patterns travel further than this then it is viewed as a pattern dying and a new pattern being created.
//If this is too small, spurios pattern annhilation / creation events will occur
//If this is too large the opposite occurs - you might merge a new pattern in with an old one.
[<Literal>] 
let maxdist=49.0f 
let inline doubleApply f (a,b) = f a,f b
let inline (---) (a,b) (d,e) = a-d,b-e
let inline ListRemove (ls:HS<_>) elem =ls.Remove(elem) |> ignore
//returns the shortest vector between 2 points (using wrapping)
let newminus a b = //formula is wrong (some terms with wrong sign) but this is irrelevant as we square later on
    let inline fixnumber n = if n > (float32 Size/2.0f) then (float32 Size) - n else if n < (-float32 Size/2.0f) then n + float32 Size else n
    a --- b |> doubleApply fixnumber
let inline dist (a,b) = a*a+b*b
let inline wdist c d = newminus c d |> dist

let trackwave_ (wave:Wave) (lst:HS<Location>) :( Wave * bool) = //wave is the old wave, list is the list of new waves
    if lst.Count > 0 then
        let closest = lst |> Seq.minBy(fun x -> wdist x (List.head wave))
        if wdist (closest) (List.head wave) < maxdist then
            ListRemove lst closest
            ((modWave wave closest),true)
        else 
            (wave,false)
    else (wave,false)

let readline (parsed:System.IO.StreamReader) =
    match parsed.ReadLine() with
    |null -> [||]
    |t->t.Split(';')
        |> Array.filter (fun t -> t <> "")
        |> Array.choose (fun t -> 
                match t.Split(',') with
                |[|a;b|] -> Some(float32 a,float32 b)
                |_ ->     None  
                )
let trackwave  (oldPts: LL<_> ) (parsed: System.IO.StreamReader) =
    if oldPts.Count = 0 then //if no previous waves 0 start from scratch
            LL<_>(readline parsed |> Array.map(MakeWave)),LL<_>() 
    else
        let mutable remaining =HS<Location>(readline parsed)
        let outputT = LL<_>()
        let outputF = LL<_>()
        //this is very non- functional but I can't see a functional way to do it, there is just too much state to hide
        for wave in oldPts do
            let wave,t = trackwave_ wave remaining
            match t with
            |true -> outputT.Add(wave) |> ignore
            |false ->outputF.Add(wave) |> ignore 
        for x in remaining do //the rest of the waves are still alive
            outputT.Add(MakeWave(x))
        outputT,outputF
let GetAllWaves (parsed:System.IO.StreamReader) fname= 
    let rec func i (old: LL<_> ) (dead:LL<_>)= 
//        if (i%100)=0 then printfn "getting waves %i" i
        let firing,newdead = trackwave  old parsed
        if i > length then //we are done now
            if false then
                use output = System.IO.File.CreateText(sprintf "out/%s.cross" fname)
                output.AutoFlush <- true
                newdead
                |> Seq.iter (fun wave ->
                    if wave |> List.length > 20 then
                        wave |> List.rev |> List.iter (fun (x,_) -> fprintf output "%f," x)
                        fprintfn output "")
                firing
                |> Seq.iter (fun wave ->
                    if wave |> List.length > 20 then
                        wave |> List.rev |> List.iter (fun (x,_) -> fprintf output "%f," x)
                        fprintfn output "")

            dead.AddRange(newdead  |> Seq.map (Array.ofList)) |> ignore
            dead.AddRange(firing |> Seq.map (Array.ofList)) |> ignore
            dead.ToArray() //add all waves and return
        else
            dead.AddRange(newdead  |> Seq.map (Array.ofList)) |> ignore
            func (i+1) firing dead //and recurse
    for i in 0..min do parsed.ReadLine() |> ignore
    func min (LL<_>())(LL<_>())
    

//optimisation idea (should work):
//take validwaves - sort by wavelength then take a slice (somehow - has to be done without copying - could just use a for loop)
//sorting must happen outside this function - in print
let MSD tau (waves)= 
    let validwaves = waves |> Array.filter(fun x ->Array.length x > tau) //it is worth doing this filtering as we can cut things out - we return validwaves
    if validwaves <> [||] then
        let bigave,sqsum,total = 
            validwaves 
            |> Array.fold (fun (sum,sqs,count) center ->
            //basically we sum over the distances by looking at dist (center.[0],center.[tau]) + (dist(center.[1],center.[tau+1]) + ... until we tun out of array entries
                let len = center |> Array.length
                let mutable smallind = 0
                let mutable bigind = tau
                let mutable out = sum
                let mutable sqsum = sqs
                let mutable ccount=count
                while bigind < len do
                    let pt1,pt2 = center.[smallind],center.[bigind]
                    let distance = wdist pt1 pt2
                    sqsum <- sqsum + (distance*distance |> float)
                    out <- distance+out
                    smallind <- smallind + 1
                    bigind <- bigind + 1
                    ccount <- ccount + 1
                out,sqsum,ccount ) (0.0f,0.0,0)
        let average = bigave / (float32 total) |> float
        let sqave=sqsum/(float total)
        let error = sqrt(sqave - average*average)/sqrt(float total)
        if System.Double.IsNaN(average) then //something bad has happened
            printfn "NAN with bigave %f total %i" bigave total
        if total > 5 then average,error,validwaves else 0.0,0.0,validwaves
    else 0.0,0.0,validwaves
let print (fname:string) parsed= 
    //printfn "running calcs for %s" fname
    let output = System.IO.File.CreateText(fname.Replace("data","out"))
    output.AutoFlush <- true
    let waves = ref ((GetAllWaves parsed fname) |> Array.filter (fun t -> t |> Array.length > 1))
    [|1..maxtau|]
    |> Array.iter (fun x ->
                        let r,e,w = MSD x (!waves)
                        waves:=w
                        if (!waves).Length > 0 then
                            fprintfn output "%f,%f" r e
                  ) 
let calc () = 
    let dirs = 
        let files = 
           System.IO.Directory.GetFiles("data")
        files 
        |> Array.iter (fun t ->System.IO.StreamReader(t) |> print t; ) 
    dirs
let calc_ (f:string) = 
    let newf = (f.Replace("input","data"))
    System.IO.StreamReader(newf) |> print newf; 
[<EntryPoint>]
let main args=
    match args with 
    |[||] -> 
        calc() |> ignore
    |[|a|] -> calc_ a
    0
