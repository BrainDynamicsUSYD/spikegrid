module Arrayops
open Params
let iterArr (f: int -> int -> 't -> unit) (arr:'t[])  =
    for i in 0..(size-1) do
        for j in 0..(size-1) do
            f i j arr.[i*size+j]

let modArr (f: int -> int -> 't -> 't) (arr:'t[])  =
    for i in 0..(size-1) do
        for j in 0..(size-1) do
            arr.[i*size+j] <- f i j arr.[i*size+j]
let zeroArr = modArr (fun _ _ _ -> 0.0f)
let inline sumArr (arr:float32[]) = 
    let mutable ret = 0
    for i in 0..(size*size-1) do
            if arr.[i] > 1.0f then ret <- ret + 1
    ret
