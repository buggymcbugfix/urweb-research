(* Copyright (c) 2009, Adam Chlipala
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - The names of contributors may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *)

structure Export :> EXPORT = struct

open Print.PD
open Print

datatype effect =
         ReadOnly
       | ReadCookieWrite
       | ReadWrite

(* What a handler is reached as, and the monad its function runs in: a
 * transaction, or, for the kinds named Io, an io computation. *)
datatype export_kind =
         Link of effect
       | Action of effect
       | Rpc of effect
       | Extern of effect
       | IoLink of effect
       | IoAction of effect
       | IoRpc of effect
       | IoExtern of effect

fun isRpc (Rpc _) = true
  | isRpc (IoRpc _) = true
  | isRpc _ = false

fun isLink (Link _) = true
  | isLink (IoLink _) = true
  | isLink _ = false

fun isAction (Action _) = true
  | isAction (IoAction _) = true
  | isAction _ = false

fun isIo (IoLink _) = true
  | isIo (IoAction _) = true
  | isIo (IoRpc _) = true
  | isIo (IoExtern _) = true
  | isIo _ = false

fun effect ek =
    case ek of
        Link ef => ef
      | Action ef => ef
      | Rpc ef => ef
      | Extern ef => ef
      | IoLink ef => ef
      | IoAction ef => ef
      | IoRpc ef => ef
      | IoExtern ef => ef

fun withEffect (ek, ef) =
    case ek of
        Link _ => Link ef
      | Action _ => Action ef
      | Rpc _ => Rpc ef
      | Extern _ => Extern ef
      | IoLink _ => IoLink ef
      | IoAction _ => IoAction ef
      | IoRpc _ => IoRpc ef
      | IoExtern _ => IoExtern ef

fun p_effect ef =
    case ef of
        ReadOnly => string "r"
      | ReadCookieWrite => string "rcw"
      | ReadWrite => string "rw"

fun p_export_kind ck =
    case ck of
        Link ef => box [string "link(", p_effect ef, string ")"]
      | Action ef => box [string "action(", p_effect ef, string ")"]
      | Rpc ef => box [string "rpc(", p_effect ef, string ")"]
      | Extern ef => box [string "extern(", p_effect ef, string ")"]
      | IoLink ef => box [string "io_link(", p_effect ef, string ")"]
      | IoAction ef => box [string "io_action(", p_effect ef, string ")"]
      | IoRpc ef => box [string "io_rpc(", p_effect ef, string ")"]
      | IoExtern ef => box [string "io_extern(", p_effect ef, string ")"]

end
