open Files
open Lexer
open Parser

type value =
  | VInt of int
  | VBool of bool
  | VString of string
  | VFlt of float
  | VTuple of value list
  | VStruct of langtype * ((Lexer.ident * value) list)
  | VFn of (param list) * langtype * expr option * ((Lexer.ident * value) list)

let delim_to_str left ls delimiter to_str right =
  left ^ 
  snd(List.fold_left (
      fun (first, acc) elem -> 
        if first then 
          (false, to_str elem)
        else
          (false, acc ^ delimiter ^ (to_str elem)))
      (true, "")
      ls) ^ 
  right

let rec langtype_to_string l =
  match l with
  | Int -> "int"
  | Float -> "float"
  | Bool -> "bool"
  | String -> "string"
  | Tuple ls -> delim_to_str "(" ls ", " langtype_to_string ")"
  | DefType t -> t

let param_to_string (par : param) : string = (fst par) ^ " : " ^ (langtype_to_string (snd par))

let rec value_to_string v =
  match v with
  | VInt i -> string_of_int i
  | VBool b -> string_of_bool b
  | VString s -> s
  | VFlt f -> string_of_float f
  | VTuple t -> delim_to_str "(" t ", " value_to_string ")"
  | VFn (params, out_type, _, _) ->  "fn " ^ (delim_to_str "(" params ", " param_to_string ")") ^ " -> " ^ (langtype_to_string out_type)
  | VStruct (langt, t) -> (langtype_to_string langt) ^ (delim_to_str "{" t ", " valued_param_to_string "}")
and valued_param_to_string (p : (Lexer.ident * value)) =
  let (ident, v) = p in ident ^ " : " ^ (value_to_string v)

let rec int_pow a = function
  | 0 -> 1
  | 1 -> a
  | n -> 
    let b = int_pow a (n / 2) in
    b * b * (if n mod 2 = 0 then 1 else a)

let rec get_first_binding s bindings : value =
  match bindings with 
  | [] -> failwith ("binding " ^ s ^ " was not found")
  | (ident, v) :: tl -> 
    if s = ident then
      v
    else 
      get_first_binding s tl

let rec multiply_str a b = if b <= 0 then "" else a ^ (multiply_str a (b - 1))

let rec eval  (env : ((Lexer.ident * value) list)) (ident : Lexer.ident)  (expression : expr) : value =
  match expression with
  | Literal value -> 
    (match value with
     | Int i -> VInt i
     | Bool b -> VBool b
     | String s -> VString s
     | Flt f -> VFlt f)
  | Let ((ident, body), next) -> 
    let new_binding = (ident, eval env ident body) in
    eval (new_binding :: env) "" next
  | FnDef (params, out_type, body) -> VFn (params, out_type, Some body, env)
  | StructDef (params) -> 
    if ident = "" then
     failwith "cannot define a struct with no name"
    else
     VFn (params, DefType ident, None, [])
  | IfElse (cond, if_body, else_body) ->
    (match (eval env "" cond) with
     | VBool b -> if b then (eval env "" if_body) else (eval env "" else_body)
     | _ -> failwith "condition isn't a boolean")
  | Var v -> get_first_binding v env
  | FnCall (fn_name, params_to_eval) -> (
      match get_first_binding fn_name env with
      | VFn (params, out_type, body, fn_env) -> 
        let new_binding_names = List.map (fst) params in 
        let params_eval = List.map (eval env "") params_to_eval in
        let new_bindings = List.combine new_binding_names params_eval in
        (
          match body with 
          | Some(body) -> 
            (
              let rec_binding = (fn_name, VFn (params, out_type, Some body, fn_env)) in
              eval (rec_binding :: (new_bindings @ fn_env)) "" body
            )
          | None -> VStruct (out_type, new_bindings)
        )
      | _ -> failwith "you're trying to call something that isn't a function"
    )
  | BinOp (fst_expr, binop, snd_expr) -> 
    (
      let fst_eval = eval env "" fst_expr in
      let snd_eval = eval env "" snd_expr in
      match (fst_eval, snd_eval, binop) with
      | (VInt a, VInt b, BinAdd) -> VInt (a + b)
      | (VInt a, VInt b, BinSub) -> VInt (a - b)
      | (VInt a, VInt b, BinMul) -> VInt (a * b)
      | (VInt a, VInt b, BinDiv) -> VInt (a / b)
      | (VInt a, VInt b, BinPower) -> VInt (int_pow a b)
      | (VInt a, VInt b, BinMod) -> VInt (a mod b)
      | (VInt a, VInt b, BinEq) -> VBool (a = b)

      | (VFlt a, VFlt b, BinAdd) -> VFlt (a +. b)
      | (VFlt a, VFlt b, BinSub) -> VFlt (a -. b)
      | (VFlt a, VFlt b, BinMul) -> VFlt (a *. b)
      | (VFlt a, VFlt b, BinDiv) -> VFlt (a /. b)
      | (VFlt a, VFlt b, BinPower) -> VFlt (a ** b)

      | (VString a, VString b, BinAdd) -> VString (a ^ b)
      | (VString a, VInt b, BinMul) -> VString (multiply_str a b)

      | (VBool a, VBool b, BinAnd) -> VBool (a && b)
      | (VBool a, VBool b, BinOr) -> VBool (a || b)

      | _ -> failwith "unsupported operation"
    )
  | SingleOp (op, expression) -> 
    let evaled = eval env "" expression in
    match (op, evaled) with 
    | (SingleNot, VBool b) -> VBool (not b)
    | (SingleNegate, VInt i) -> VInt(-i)
    | (SingleNegate, VFlt f) -> VFlt(-.f)
    | _ -> failwith "unsupported operation"

let run file = 
  let tokenized = tokenize (read_whole_file file) in
  let parsed = parse tokenized in
  let v = eval [] "" parsed in
  v

let () = Printf.printf "%s\n" (value_to_string (run Sys.argv.(1)))