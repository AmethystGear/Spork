open Lexer

type langtype =
  | Int
  | Bool
  | Float
  | String
  | Tuple of langtype list
  | DefType of Lexer.ident

type param = (Lexer.ident * langtype)

type binop =
  | BinEq
  | BinAdd
  | BinSub
  | BinPower
  | BinMul
  | BinDiv
  | BinAnd
  | BinOr
  | BinMod

type singleop =
  | SingleNegate
  | SingleNot

type expr =
  | Literal of Lexer.literal
  | Let of (Lexer.ident * expr) * expr
  | FnDef of (param list) * langtype * expr
  | StructDef of (param list)
  | IfElse of (expr * expr * expr)
  | Var of Lexer.ident
  | FnCall of (Lexer.ident * (expr list))
  | BinOp of (expr * binop * expr)
  | SingleOp of (singleop * expr)


let op_to_singleop op =
  match op with
  | Not -> Some SingleNot
  | Sub -> Some SingleNegate
  | _ -> None

let op_to_binop op =
  match op with
  | Eq -> Some BinEq
  | Add -> Some BinAdd
  | Sub -> Some BinSub
  | Power -> Some BinPower
  | Mul -> Some BinMul
  | Div -> Some BinDiv
  | And -> Some BinAnd
  | Or -> Some BinOr
  | Mod -> Some BinMod
  | _ -> None

let rec parse_delimited (fn : (token list -> ('a  * (token list)) option)) (delim : tokentype) (terminator : tokentype) (tokens : token list) :  (('a list) * (token list)) option =
  match fn tokens with
  | None -> None
  | Some (value, new_tokens) -> 
    match new_tokens with
    | [] -> None
    | (hd, _, _) :: tl -> 
      if hd = delim then 
        match parse_delimited fn delim terminator tl with
        | Some (delimited, remaining_tokens) -> Some (value :: delimited, remaining_tokens)
        | None -> None
      else if hd = terminator then
        Some ([value], tl)
      else
        None

let rec parse_type (tokens : token list) : (langtype * (token list)) option =
  match tokens with
  | [] -> None
  | (hd, _, _) :: tl ->
    match hd with
    | Lexer.Punc Lexer.LeftParen -> 
      (let delim_type = parse_delimited parse_type (Lexer.Punc Lexer.Delim) (Lexer.Punc Lexer.RightParen) tl in
       match delim_type with
       | Some (delimited, remaining) -> Some (Tuple delimited, remaining)
       | None -> None)
    | Lexer.Ident str -> 
      (let lang_type = match str with
          | "int" -> Some Int
          | "bool" -> Some Bool
          | "float" -> Some Float
          | "string" -> Some String
          | _ -> Some (DefType str)
       in
       match lang_type with
       | Some(lt) -> Some(lt, tl)
       | None -> None)
    | _ -> None

let parse_param (tokens : token list) : (param * (token list)) option =
  match tokens with
  | ((Lexer.Ident ident), _, _) :: ((Lexer.Punc Lexer.TypeSpecifier), _, _) :: tl -> 
    (match parse_type tl with 
     | Some (lang_type, token_list) -> Some((ident, lang_type), token_list)
     | _ -> None)
  | _ -> None

let parse_params_paren (tokens : token list) : ((param list) * (token list)) option =
  match tokens with
  | ((Lexer.Punc Lexer.LeftParen), _, _) :: tl -> parse_delimited parse_param (Lexer.Punc Lexer.Delim) (Lexer.Punc Lexer.RightParen) tl
  | _ -> None

let parse_params_scope (tokens : token list) : ((param list) * (token list)) option =
  match tokens with
  | ((Lexer.Punc Lexer.LeftScope), _, _) :: tl -> parse_delimited parse_param (Lexer.Punc Lexer.Delim) (Lexer.Punc Lexer.RightScope) tl
  | _ -> None

let rec parse_expr (tokens : token list) : (expr * (token list)) =
  match tokens with
  | [] -> failwith "tokens list terminated early!"
  | (token, col, line) :: tl ->
    let expression = match token with
      | (Lexer.Kw Lexer.Let) -> parse_let tokens
      | (Lexer.Kw Lexer.Fn) -> parse_fn_def tokens
      | (Lexer.Kw Lexer.If) -> parse_ifelse tokens
      | (Lexer.Kw Lexer.Struct) -> parse_struct tokens
      | (Lexer.Lit _) -> parse_lit tokens
      | (Lexer.Ident ident) -> (match parse_fncall tokens with Some(v) -> Some(v) | None -> Some(Var ident, tl))
      | (Lexer.Op op) -> (match op_to_singleop op with Some _ -> parse_singleop tokens | None -> None)
      | (Lexer.Punc Lexer.LeftParen) -> parse_binop tokens
      | _ -> None 
    in
    match expression with
    | Some (expression) -> expression
    | None -> failwith ("Could not parse expression at line: " ^ string_of_int(line) ^ " col: " ^ string_of_int(col))

and parse_struct (tokens : token list) : (expr * (token list)) option =
  match tokens with
  | ((Lexer.Kw Lexer.Struct), _, _) :: tl ->
    (
      match (parse_params_scope tl) with
      | Some (params, tl) -> Some ((StructDef params), tl)
      | _ -> None
    )
  | _ -> None

and parse_fn_def (tokens : token list) : (expr * (token list)) option =
  match tokens with
  | ((Lexer.Kw Lexer.Fn), _, _) :: tl -> 
    (
      match parse_params_paren tl with
      | Some (params, ((Lexer.Op Lexer.Into), _, _) :: tl) -> 
        (
          match parse_type tl with
          | Some (lang_type, ((Lexer.Punc Lexer.LeftScope), _, _) :: tl) -> 
            (
              match parse_expr tl with 
              | (body, ((Lexer.Punc Lexer.RightScope), _, _) :: tl) -> Some (FnDef (params, lang_type, body), tl)
              | _ -> None
            )
          | _ -> None
        )
      | _ -> None
    )
  | _ -> None

and parse_lit (tokens : token list) : (expr * (token list)) option = 
  match tokens with
  | ((Lit lit), _, _) :: tl -> Some (Literal lit, tl)
  | _ -> None

and parse_let (tokens : token list) : (expr * (token list)) option = 
  match tokens with
  | ((Lexer.Kw Lexer.Let), _, _) :: ((Lexer.Ident ident), _, _) :: ((Lexer.Op Lexer.Assign), _, _) :: tl -> 
    (
      match parse_expr tl with
      | (body, ((Lexer.Punc Lexer.Terminator), _, _) :: tl) -> 
        (
          let (next_expr, tl) = parse_expr tl in Some (Let ((ident, body), next_expr), tl)
        )
      | _ -> None
    )
  | _ -> None

and parse_ifelse (tokens : token list) : (expr * (token list)) option = 
  match tokens with
  | ((Lexer.Kw Lexer.If), _, _) :: tl -> 
    (
      match parse_expr tl with
      | (cond, ((Lexer.Punc Lexer.LeftScope), _, _) :: tl) ->
        (
          match parse_expr tl with
          | (if_body, ((Lexer.Punc Lexer.RightScope), _, _) :: ((Lexer.Kw Lexer.Else), _, _) :: ((Lexer.Punc Lexer.LeftScope), _, _) :: tl) ->
            (
              match parse_expr tl with
              | (else_body, ((Lexer.Punc Lexer.RightScope), _, _) :: tl) -> Some(IfElse(cond, if_body, else_body), tl)
              | _ -> None
            )
          | _ -> None
        )
      | _ -> None
    )
  | _ -> None

and parse_fncall (tokens : token list) : (expr * (token list)) option = 
  match tokens with
  | ((Ident ident), _, _) :: ((Lexer.Punc Lexer.LeftParen), _, _) :: tl -> 
    (
      match parse_delimited (fun e -> Some (parse_expr e)) (Lexer.Punc Lexer.Delim) (Lexer.Punc Lexer.RightParen) tl with
      | Some (params, tl) -> Some (FnCall (ident, params), tl)
      | _ -> None
    )
  | _ -> None

and parse_singleop (tokens : token list) :  (expr * (token list)) option = 
  match tokens with
  | ((Lexer.Op hd), _, _) :: tl -> 
    (
      match (op_to_singleop hd, parse_expr tl) with
      | (Some single_op, (expr, tl)) -> Some (SingleOp (single_op, expr), tl)
      | _ -> None
    )
  | _ -> None

and parse_binop (tokens : token list) : (expr * (token list)) option =
  match tokens with
  | ((Lexer.Punc Lexer.LeftParen), _, _) :: tl ->
    (
      let (fst_expr, tl) = parse_expr tl in
      match tl with
      | ((Lexer.Punc Lexer.RightParen), _, _) :: ((Lexer.Op hd), _, _) :: ((Lexer.Punc Lexer.LeftParen), _, _) :: tl ->
        (
          match (op_to_binop hd, parse_expr tl) with
          | (Some binop, (snd_expr,  ((Lexer.Punc Lexer.RightParen), _, _) :: tl)) -> Some(BinOp (fst_expr, binop, snd_expr), tl)
          | _ -> None
        )
      | _ -> None
    )
  | _ -> None

let parse tokens = match parse_expr tokens with (expr, []) -> expr | _ -> failwith "expected EOF, found "

let test_type = parse_type(tokenize "(int, int, (bool, int))")
let test_code = parse_expr(tokenize "let x = fn (a : int) -> int { ((a) + (a)) + ((a) + (a)) }; x(5)")
let test_1 = parse_expr(tokenize "let A = struct { x : int, y : int }; A(0, 1)")