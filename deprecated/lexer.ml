type punc = 
  | LeftParen
  | RightParen
  | LeftScope
  | RightScope
  | TypeSpecifier
  | Delim
  | Terminator

type op =
  | Eq
  | Assign
  | Into
  | Add
  | Sub
  | Power
  | Mul
  | Div
  | Not
  | And
  | Or
  | Mod

type kw =
  | Fn
  | Let
  | If
  | Else
  | Type
  | Struct
  | Variant
type literal =
  | Int of int
  | Flt of float
  | Bool of bool
  | String of string

type ident = string

type tokentype =
  | Punc of punc
  | Op of op
  | Kw of kw
  | Lit of literal
  | Ident of ident

type token = tokentype * int * int

let is_digit (chr : char) : bool =
  match chr with
  | '0' | '1' | '2' | '3' | '4' | '5' | '6' | '7' | '8' | '9' -> true
  | _ -> false

let is_whitespace (chr : char) : bool =
  match chr with
  | ' ' | '\n' | '\r' | '\t' -> true
  | _ -> false

(* exactly match the char list to a string. If the string doesn't match, then return None. 
   otherwise, return Some(ret, xs with the provided string removed from the front) *)

(* ex.
    (match_exact ['a', 'b', 'c', 'd'] ("ab", 1)) would return Some(1, ['c', 'd'])
    (match_exact ['a', 'b', 'c', 'd'] ("cb", 1)) would return None
*)
let match_exact ((s : string), (ret : 'a)) (xs : char list)  : ('a * char list * int) option  =
  let (matched, new_xs) = List.fold_left (
      fun (matching, new_xs) elem -> 
        if matching then 
          match new_xs with
          | [] -> (false, new_xs)
          | hd :: tl -> 
            if hd = elem then
              (true, tl)
            else
              (false, [])
        else
          (false, [])
    ) (true, xs) (List.of_seq (String.to_seq s)) in 
  if matched then
    Some(ret, new_xs, String.length s)
  else
    None

(* exactly match the first pair in the matches list *)
let match_first (xs : char list) (matches : (char list -> ('a * char list * int) option) list) : ('a * char list * int) option =
  let v = List.filter_map (fun elem -> elem xs) matches in
  match v with 
  | [] -> None
  | hd :: tl -> Some(hd)

let match_punc (xs : char list) : (punc * char list * int) option =
  let punc =  List.map match_exact [
      ("(", LeftParen); 
      (")", RightParen); 
      ("{", LeftScope); 
      ("}", RightScope);
      (":", TypeSpecifier);  
      (",", Delim); 
      (";", Terminator)
    ] in
  match_first xs punc

let match_ops (xs : char list) : (op * char list * int) option =
  let ops = List.map match_exact [
      ("==", Eq); 
      ("=", Assign); 
      ("->", Into); 
      ("+", Add);
      ("-", Sub);  
      ("**", Power); 
      ("*", Mul);
      ("/", Div);
      ("!", Not); 
      ("&&", And);
      ("||", Or);
      ("%", Mod)
    ] in
  match_first xs ops

let match_exact_with_space ((s : string), (ret : 'a)) (xs : char list)  : ('a * char list * int) option  =
  let v = match_exact (s, ret) xs in
  match v with 
  | Some (m, hd :: tl, _) -> 
    if is_whitespace(hd) || Option.is_some (match_punc (hd :: tl))  || Option.is_some (match_ops (hd :: tl)) then
      v
    else 
      None
  | _ -> None

let match_bool (xs : char list) : (literal * char list * int) option =
  match_first xs (List.map match_exact [("true", Bool true); ("false", Bool false)])

let match_int (xs : char list) : (literal * char list * int) option =
  let rec int_match_helper acc consumed xs =
    match xs with 
    | [] -> (acc, [], consumed)
    | hd :: tl -> 
      if is_digit hd then
        int_match_helper (acc ^ (String.make 1 hd)) (consumed + 1) tl
      else
        (acc, hd :: tl, consumed)
  in
  match int_match_helper "" 0 xs with
  | ("", _, _) -> None
  | (str, new_xs, consumed) -> Some (Int (int_of_string str), new_xs, consumed)

let match_flt (xs : char list) : (literal * char list * int) option =
  let rec flt_match_helper acc consumed has_dec xs =
    match xs with 
    | [] -> (Some acc, [], consumed)
    | hd :: tl -> 
      match (is_digit(hd), hd = '.', has_dec) with 
      | (true, _, _) -> flt_match_helper  (acc ^ (String.make 1 hd)) (consumed + 1) has_dec tl
      | (false, true, false) -> flt_match_helper (acc ^ (String.make 1 hd)) (consumed + 1) true tl
      | (false, true, true) -> (None, [], consumed)
      | (false, false, _) -> (Some acc, xs, consumed)
  in
  match flt_match_helper "" 0 false xs with
  | (None, _,_) | (Some "", _, _) -> None
  | (Some str, new_xs, consumed) -> Some (Flt (float_of_string str), new_xs, consumed)

let match_str (xs : char list) : (literal * char list * int) option =
  let rec str_match_helper acc consumed escape xs =
    match xs with 
    | [] -> (None, [], consumed)
    | hd :: tl -> 
      match (hd, escape) with
      | ('"', false) -> (Some acc, tl, consumed + 1)
      | ('\\', _) -> str_match_helper (acc ^ (String.make 1 hd)) (consumed + 1) (not escape) tl
      | (c, _) -> str_match_helper (acc ^ (String.make 1 hd)) (consumed + 1) (false) tl
  in
  match xs with
  | [] -> None
  | hd :: tl ->
    if hd = '"' then
      match str_match_helper "" 0 false tl with
      | (Some str, new_xs, consumed) -> Some (String str, new_xs, consumed)
      | (None, _, _) -> None
    else
      None

let match_literal (xs : char list) : (literal * char list * int) option =
  let literals = [
    match_int;
    match_bool;
    match_str;    
    match_flt;    
  ] in
  match_first xs literals

let match_kws (xs : char list) : (kw * char list * int) option =
  let ops = List.map match_exact_with_space [
      ("fn", Fn); 
      ("let", Let); 
      ("if", If); 
      ("else", Else);
      ("struct", Struct);
      ("variant", Variant);
    ] in
  match_first xs ops

let match_ident (xs : char list) : (ident * char list * int) option =
  let rec ident_helper acc consumed xs =
    match xs with
    | [] -> (acc, [], consumed)
    | hd :: tl ->
      if is_whitespace hd || Option.is_some (match_punc xs) || Option.is_some (match_ops xs) then 
        (acc, xs, consumed)
      else 
        ident_helper (acc ^ (String.make 1 hd)) (consumed + 1) tl
  in

  match ident_helper "" 0 xs with
  | ("", _, _) -> None
  | (str, new_xs, consumed) -> Some (str, new_xs, consumed)

let match_token (xs : char list) : (tokentype * char list * int) option =
  let mapper f a = 
    fun elems ->
      match f elems with
      | Some (v, new_elems, consumed) -> Some ((a v), new_elems, consumed)
      | None -> None
  in

  let token_match = [
    (mapper match_punc (fun punc -> Punc punc));
    (mapper match_ops (fun op -> Op op));
    (mapper match_literal (fun lit -> Lit lit));
    (mapper match_kws (fun kw -> Kw kw));
    (mapper match_ident (fun id -> Ident id));
  ] in

  match_first xs token_match

let tokenize (s : string) : (token list) =
  let rec discard_till_next_line xs =
    match xs with
    | [] -> []
    | hd :: tl -> if hd = '\n' then tl else discard_till_next_line tl
  in

  let rec tokenize_helper col line acc xs : (token list) =
    match xs with
    | [] -> List.rev acc
    | hd :: tl ->
      if hd = '\n' then 
        tokenize_helper (0) (line + 1) acc tl
      else if is_whitespace hd then
        tokenize_helper (col + 1) line acc tl
      else if hd = '#' then
        tokenize_helper (0) (line + 1) acc (discard_till_next_line tl)
      else
        match match_token xs with
        | Some(token, remaining, consumed) -> 
          let token = (token, col, line) in
          tokenize_helper (col + consumed) line (token :: acc) remaining
        | None -> failwith ("failed to parse token at line: "  ^ string_of_int(line) ^ " col: " ^  string_of_int(col) ^ "\n")
  in
  tokenize_helper 0 1 [] (List.of_seq (String.to_seq s))

let test1 = tokenize "let x = 5;"
let test2 = tokenize "let x = true; let y = false;"
let test3 = tokenize "let add= fn (a : int, b : int) -> int { a + b };"
let test4 = tokenize "let A = type struct { test_1 : int, test_2 : int };"